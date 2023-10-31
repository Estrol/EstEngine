#ifndef __OPENGLBACKEND_H_
#define __OPENGLBACKEND_H_
#include "./glad/gl.h"
#include <Graphics/GraphicsBackendBase.h>
#include <map>
#include <vector>

namespace Graphics {
    namespace Backends {
        struct ShaderData
        {
            GLuint vert;
            GLuint frag;
            GLuint program;
        };

        struct OpenGLData
        {
            void                                    *ctx;
            GLuint                                   vertexBuffer;
            GLuint                                   indexBuffer;
            GLuint                                   constantBuffer;
            GLuint                                   maxVertexBufferSize;
            GLuint                                   maxIndexBufferSize;
            std::map<ShaderFragmentType, ShaderData> shaders;
        };

        class OpenGL : public Base
        {
        public:
            virtual ~OpenGL() = default;

            virtual void Init() override;
            virtual void ReInit() override;
            virtual void Shutdown() override;

            virtual bool NeedReinit() override;

            virtual bool BeginFrame() override;
            virtual void EndFrame() override;

            virtual void ImGui_Init() override;
            virtual void ImGui_DeInit() override;
            virtual void ImGui_NewFrame() override;
            virtual void ImGui_EndFrame() override;

            virtual void Push(SubmitInfo &info) override;

            virtual void SetClearColor(glm::vec4 color) override;
            virtual void SetClearDepth(float depth) override;
            virtual void SetClearStencil(uint32_t stencil) override;

            virtual BlendHandle CreateBlendState(TextureBlendInfo blendInfo) override;

            GLuint CreateTexture();
            void   DestroyTexture(GLuint texture);

        private:
            void CreateShader();
            void CreateDefaultBlend();

            void       FlushQueue();
            OpenGLData Data;

            std::vector<SubmitInfo>                 submitInfos;
            std::vector<GLuint>                     textures;
            std::map<BlendHandle, TextureBlendInfo> blendStates;
        };
    } // namespace Backends
} // namespace Graphics

#endif