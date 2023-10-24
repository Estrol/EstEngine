#include <Graphics/Renderer.h>
#include <UI/Image.h>
using namespace UI;
using namespace Graphics;

Image::Image()
{
}

Image::Image(std::filesystem::path path)
{
    auto renderer = Renderer::Get();
    auto image = renderer->LoadTexture(path);

    m_texture = std::unique_ptr<Texture2D>(image);
}

Image::Image(const char *buf, size_t size)
{
    auto renderer = Renderer::Get();
    auto image = renderer->LoadTexture(buf, size);

    m_texture = std::unique_ptr<Texture2D>(image);
}

Image::Image(const char *pixbuf, uint32_t width, uint32_t height)
{
    auto renderer = Renderer::Get();
    auto image = renderer->LoadTexture(pixbuf, width, height);

    m_texture = std::unique_ptr<Texture2D>(image);
}

void Image::OnDraw()
{
    using namespace Backends;
    CalculateSize();

    double x1 = AbsolutePosition.X;
    double y1 = AbsolutePosition.Y;
    double x2 = x1 + AbsoluteSize.X;
    double y2 = y1 + AbsoluteSize.Y;

    glm::vec2 uv1(0.0f, 0.0f); // Top-left UV coordinate
    glm::vec2 uv2(1.0f, 0.0f); // Top-right UV coordinate
    glm::vec2 uv3(1.0f, 1.0f); // Bottom-right UV coordinate
    glm::vec2 uv4(0.0f, 1.0f); // Bottom-left UV coordinate

    if (m_vertices.size() != 6) {
        m_vertices.resize(6);
        m_indices = { 0, 1, 2, 3, 4, 5 };
    }

    glm::vec4 color = {
        Color3.R * 255,
        Color3.G * 255,
        Color3.B * 255,
        Transparency * 255
    };

    shaderFragmentType = ShaderFragmentType::Image;

    for (int i = 0; i < 6; i++) {
        Vertex &vertex = m_vertices[i];
        vertex.SetColorRGB(color);

        switch (i) {
            case 0:
                vertex.pos = glm::vec2(x1, y1);
                vertex.texCoord = uv1;
                break;
            case 1:
                vertex.pos = glm::vec2(x2, y1);
                vertex.texCoord = uv2;
                break;
            case 2:
                vertex.pos = glm::vec2(x2, y2);
                vertex.texCoord = uv3;
                break;
            case 3:
                vertex.pos = glm::vec2(x1, y1);
                vertex.texCoord = uv1;
                break;
            case 4:
                vertex.pos = glm::vec2(x2, y2);
                vertex.texCoord = uv3;
                break;
            case 5:
                vertex.pos = glm::vec2(x1, y2);
                vertex.texCoord = uv4;
                break;
        }
    }
}