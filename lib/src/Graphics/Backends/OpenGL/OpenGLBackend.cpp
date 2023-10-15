
#define NOMINMAX
#include "OpenGLBackend.h"

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include "../../Shaders/image.spv.h"
#include "../../Shaders/position.spv.h"
#include "../../Shaders/solid.spv.h"
#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#include <algorithm>

#include <spirv_cross/spirv_glsl.hpp>

using namespace Graphics::Backends;

std::string compileSPRIV(const uint32_t* data, size_t size)
{
    spirv_cross::CompilerGLSL compiler(data, size);

    spirv_cross::CompilerGLSL::Options options;
	options.version = 310;
	compiler.set_common_options(options);

    return compiler.compile();
    // return "";
}

void OpenGL::Init()
{
    auto window = Graphics::NativeWindow::Get();
    if (SDL_GL_LoadLibrary(NULL) != 0) {
        throw Exceptions::EstException("Failed to load OpenGL library");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // create context
    SDL_GLContext context = SDL_GL_CreateContext(window->GetWindow());

    if (context == nullptr) {
        throw Exceptions::EstException("Failed to create OpenGL context");
    }

    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0) {
        throw Exceptions::EstException("Failed to load OpenGL functions");
    }

    if (!SDL_GL_ExtensionSupported("GL_ARB_gl_spirv")) {
        throw Exceptions::EstException("SPIR-V extensions support on GPU driver is required to run this software");
    }

    Data.ctx = context;

    constexpr uint32_t MAX_VERTEX_OBJECTS = 50000;
    constexpr uint32_t MAX_VERTEX_BUFFER_SIZE = sizeof(Vertex) * MAX_VERTEX_OBJECTS;
    constexpr uint32_t MAX_INDEX_BUFFER_SIZE = sizeof(uint32_t) * MAX_VERTEX_OBJECTS;

    glGenBuffers(1, &Data.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, Data.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &Data.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Data.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDEX_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

    // enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // enable texture 2d
    glEnable(GL_TEXTURE_2D);

    // set viewport
    glViewport(0, 0, window->GetWindowSize().Width, window->GetWindowSize().Height);

    std::vector<std::pair<ShaderFragmentType, std::pair<const uint32_t *, size_t>>> shaders = {
        { ShaderFragmentType::Solid, { __glsl_solid, sizeof(__glsl_solid) } },
        { ShaderFragmentType::Image, { __glsl_image, sizeof(__glsl_image) } }
    };

    for (auto &[type, shader] : shaders) {
        auto data = compileSPRIV(__glsl_position, sizeof(__glsl_position));


        GLuint shaderId = glCreateShader(GL_VERTEX_SHADER);
        glShaderBinary(1, &shaderId, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, __glsl_position, sizeof(__glsl_position));
        glSpecializeShaderARB(shaderId, "main", 0, nullptr, nullptr);

        GLint errcode = GL_TRUE;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &errcode);

        if (errcode != GL_TRUE) {
            throw Exceptions::EstException("Failed to compile vertex shader");
        }

        GLuint fragmentId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderBinary(1, &fragmentId, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, shader.first, shader.second);
        glSpecializeShaderARB(fragmentId, "main", 0, nullptr, nullptr);

        glGetShaderiv(fragmentId, GL_COMPILE_STATUS, &errcode);

        if (errcode != GL_TRUE) {
            throw Exceptions::EstException("Failed to compile fragment shader");
        }

        GLuint programId = glCreateProgram();
        glAttachShader(programId, shaderId);
        glAttachShader(programId, fragmentId);
        glLinkProgram(programId);

        glGetProgramiv(programId, GL_LINK_STATUS, &errcode);

        if (errcode != GL_TRUE) {
            throw Exceptions::EstException("Failed to link shader program");
        }

        Data.shaders[type] = { shaderId, fragmentId, programId };
    }

    Data.maxVertexBufferSize = MAX_VERTEX_BUFFER_SIZE;
    Data.maxIndexBufferSize = MAX_INDEX_BUFFER_SIZE;

    // float[2] scale, translation uniform buffer
    glGenBuffers(1, &Data.constantBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, Data.constantBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
}

void OpenGL::Shutdown()
{
    // free the buffer
    glDeleteBuffers(1, &Data.vertexBuffer);
    glDeleteBuffers(1, &Data.indexBuffer);
    glDeleteBuffers(1, &Data.constantBuffer);

    // delete shader program
    for (auto &[type, shader] : Data.shaders) {
        glDeleteProgram(shader.program);
        glDeleteShader(shader.vert);
        glDeleteShader(shader.frag);
    }

    SDL_GL_DeleteContext(Data.ctx);
}

void OpenGL::ReInit()
{
}

bool OpenGL::NeedReinit()
{
    return false;
}

bool OpenGL::BeginFrame()
{
    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();

    glViewport(0, 0, rect.Width, rect.Height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

void OpenGL::EndFrame()
{
    FlushQueue();
    SDL_GL_SwapWindow((SDL_Window *)Graphics::NativeWindow::Get()->GetWindow());
}

void OpenGL::Push(SubmitInfo &info)
{
    submitInfos.push_back(info);
}

void OpenGL::FlushQueue()
{
    if (submitInfos.size() == 0) {
        return;
    }

    GLuint vertex_size = 0;
    GLuint indices_size = 0;
    for (auto &info : submitInfos) {
        vertex_size += (GLuint)(info.vertices.size() * sizeof(info.vertices[0]));
        indices_size += (GLuint)(info.indices.size() * sizeof(info.indices[0]));
    }

    vertex_size = std::clamp((GLuint)vertex_size, (GLuint)0, (GLuint)Data.maxVertexBufferSize);
    indices_size = std::clamp((GLuint)indices_size, (GLuint)0, (GLuint)Data.maxIndexBufferSize);

    glBindBuffer(GL_ARRAY_BUFFER, Data.vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Data.indexBuffer);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_size, nullptr);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices_size, nullptr);

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    uint16_t currentVertexCount = 0; // This will be used as an offset for indices

    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();

    float scale[2];
    scale[0] = 2.0f / rect.Width;
    scale[1] = 2.0f / rect.Height;

    float translate[2];
    translate[0] = -1.0f;
    translate[1] = 0.5f;

    for (auto &info : submitInfos) { // Assuming `infos` is your std::vector<Info>
        for (auto &vertex : info.vertices) {
            vertex.scale = glm::vec2(scale[0], scale[1]);
            vertex.translate = glm::vec2(translate[0], translate[1]);
            // vertex.pos = glm::vec2(vertex.pos.x, vertex.pos.y * -1);

            vertices.push_back(vertex);
        }

        for (uint16_t index : info.indices) {
            indices.push_back(index + currentVertexCount);
        }

        currentVertexCount += (uint16_t)info.vertices.size();
    }

    GLuint vertex_offset = 0;
    GLuint indices_offset = 0;

    glBindBuffer(GL_ARRAY_BUFFER, Data.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Data.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)MY_OFFSETOF(Vertex, pos));

    // Texture coordinates attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)MY_OFFSETOF(Vertex, texCoord));

    // Color attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *)MY_OFFSETOF(Vertex, color));

    // Scale attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)MY_OFFSETOF(Vertex, scale));

    // Translate attribute
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)MY_OFFSETOF(Vertex, translate));

    for (auto &info : submitInfos) {
        auto &vertices = info.vertices;
        auto &indices = info.indices;
        auto shadertype = info.fragmentType;
        GLuint imageId = info.image;

        auto shader = Data.shaders[shadertype].program;
        glUseProgram(shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, imageId);

        GLuint firstIndex = indices_offset;
        GLuint indexCount = info.indices.size();

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(firstIndex * sizeof(uint16_t)));

        vertex_offset += (GLuint)info.vertices.size();
        indices_offset += indexCount;

        glUseProgram(0);
    }

    submitInfos.clear();
}

void OpenGL::ImGui_Init()
{
}

void OpenGL::ImGui_DeInit()
{
}

void OpenGL::ImGui_NewFrame()
{
}

void OpenGL::ImGui_EndFrame()
{
}