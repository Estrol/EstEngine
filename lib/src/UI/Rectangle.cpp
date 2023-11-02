#include <Graphics/Renderer.h>
#include <UI/Rectangle.h>

using namespace UI;
using namespace Graphics;

Rectangle::Rectangle() : Base()
{
    std::vector<char> pixels = {
        (char)0x255, (char)0x255, (char)0x255, (char)0x255
    };

    auto image_ptr = Renderer::Get()->LoadTexture(
        pixels.data(),
        1,
        1);

    m_texture = std::unique_ptr<Graphics::Texture2D>(image_ptr);
}

void Rectangle::OnDraw()
{
    using namespace Graphics::Backends;
    CalculateSize();

    double x1 = AbsolutePosition.X;
    double y1 = AbsolutePosition.Y;
    double x2 = x1 + AbsoluteSize.X;
    double y2 = y1 + AbsoluteSize.Y;

    glm::vec2 uv1(0.0f, 0.0f); // Top-left UV coordinate
    glm::vec2 uv2(1.0f, 0.0f); // Top-right UV coordinate
    glm::vec2 uv3(1.0f, 1.0f); // Bottom-right UV coordinate
    glm::vec2 uv4(0.0f, 1.0f); // Bottom-left UV coordinate
    glm::vec4 corRad(
        CornerRadius.XY.X,
        CornerRadius.XY.Y,
        CornerRadius.ZW.X,
        CornerRadius.ZW.Y);

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

    uint32_t col = ((uint32_t)(color.a) << 24) | ((uint32_t)(color.b) << 16) | ((uint32_t)(color.g) << 8) | ((uint32_t)(color.r) << 0);

    shaderFragmentType = ShaderFragmentType::Solid;

    m_vertices = {
        { { x1, y1 }, uv1, col },
        { { x1, y2 }, uv4, col },
        { { x2, y2 }, uv3, col },

        { { x1, y1 }, uv1, col },
        { { x2, y2 }, uv3, col },
        { { x2, y1 }, uv2, col },
    };
}