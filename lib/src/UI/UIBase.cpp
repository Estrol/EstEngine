#include <Exceptions/EstException.h>
#include <Graphics/GraphicsTexture2D.h>
#include <Graphics/NativeWindow.h>
#include <Graphics/Renderer.h>
#include <UI/UIBase.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

using namespace UI;

Base::Base()
{
    Position = UDim2::fromScale(0.0, 0.0);
    Size = UDim2::fromScale(1.0, 1.0);
    AnchorPoint = { 0.0f, 0.0f };
    Color3 = Color3::fromRGB(255, 255, 255);
    Transparency = 1;
    Rotation = 0;
    Parent = nullptr;
    ClampToParent = false;

    m_renderMode = RenderMode::Normal;
    m_texturePtr = nullptr;
}

Base::~Base()
{
    // Automated garbage collector!
    if (m_texture) {
        m_texture.reset();
    }
}

void Base::Draw()
{
    clipRect = Graphics::NativeWindow::Get()->GetWindowSize();
    OnDraw();
    DrawVertices();
}

void Base::Draw(Rect _clipRect)
{
    clipRect = _clipRect;
    OnDraw();
    DrawVertices();
}

void Base::CalculateSize()
{
    auto   windowRect = Graphics::NativeWindow::Get()->GetWindowSize();
    double Width = windowRect.Width;
    double Height = windowRect.Height;
    double X = 0;
    double Y = 0;

    if (Parent != nullptr && Parent != this) {
        Parent->CalculateSize();

        Width = Parent->AbsoluteSize.X;
        Height = Parent->AbsoluteSize.Y;
        X = Parent->AbsolutePosition.X;
        Y = Parent->AbsolutePosition.Y;
    }

    double x0 = Width * Position.X.Scale + Position.X.Offset;
    double y0 = Height * Position.Y.Scale + Position.Y.Offset;

    double x1 = Width * Size.X.Scale + Size.X.Offset;
    double y1 = Height * Size.Y.Scale + Size.Y.Offset;

    double xAnchor = x1 * std::clamp(AnchorPoint.X, 0.0, 1.0);
    double yAnchor = y1 * std::clamp(AnchorPoint.Y, 0.0, 1.0);

    x0 -= xAnchor;
    y0 -= yAnchor;

    AbsolutePosition = { X + x0, Y + y0 };
    AbsoluteSize = { x1, y1 };
}

void Base::DrawVertices()
{
    using namespace Graphics::Backends;
    auto renderer = Graphics::Renderer::Get();

    if (m_renderMode == RenderMode::Normal) {
        SubmitInfo info = {};
        info.clipRect = clipRect;
        info.vertices = m_vertices;
        info.fragmentType = shaderFragmentType;
        info.indices = m_indices;

        if (m_texturePtr != nullptr) {
            info.image = m_texturePtr->GetId();
        } else if (m_texture) {
            info.image = m_texture->GetId();
        }

        RotateVertex();
        renderer->Push(info);
    } else {
        if (!m_batches.size()) {
            InsertToBatch();
        }

        RotateVertex();
        for (auto &batch : m_batches) {
            renderer->Push(batch);
        }
    }
}

void Base::InsertToBatch()
{
    using namespace Graphics::Backends;
    auto renderer = Graphics::Renderer::Get();

    if (m_renderMode == RenderMode::Normal) {
        return;
    }

    SubmitInfo info = {};
    info.clipRect = clipRect;
    info.vertices = m_vertices;
    info.fragmentType = shaderFragmentType;
    info.indices = m_indices;

    if (m_texturePtr != nullptr) {
        info.image = m_texturePtr->GetId();
    } else if (m_texture) {
        info.image = m_texture->GetId();
    }

    m_batches.push_back(info);
}

inline glm::vec2 computeCenter(std::vector<Graphics::Backends::Vertex> &vertices)
{
    glm::vec2 sum(0.0f, 0.0f);
    int       count = 0;

    for (const auto &vertex : vertices) {
        sum += vertex.pos;
        count++;
    }

    return sum / static_cast<float>(count);
}

inline glm::vec2 computeCenter(const std::vector<Graphics::Backends::SubmitInfo> &batches)
{
    glm::vec2 sum(0.0f, 0.0f);
    int       count = 0;

    for (const auto &batch : batches) {
        for (const auto &vertex : batch.vertices) {
            sum += vertex.pos;
            count++;
        }
    }

    return sum / static_cast<float>(count);
}

inline glm::vec2 rotate(glm::vec2 &vec, float cos, float sin)
{
    return { vec.x * cos - vec.y * sin, vec.x * sin + vec.y * cos };
}

void Base::RotateVertex()
{
    if (!m_vertices.size()) {
        throw Exceptions::EstException("Vertex array is empty");
    }

    float radians = glm::radians(Rotation);
    float cosAngle = glm::cos(radians);
    float sinAngle = glm::sin(radians);

    if (m_renderMode == RenderMode::Normal) {
        auto center = computeCenter(m_vertices);
        center = rotate(center, cosAngle, sinAngle) - center;

        for (auto &vertex : m_vertices) {
            vertex.pos = rotate(vertex.pos, cosAngle, sinAngle) - center;
        }
    } else {
        auto center = computeCenter(m_batches);
        center = rotate(center, cosAngle, sinAngle) - center;

        for (auto &info : m_batches) {
            for (auto &vertices : info.vertices) {
                vertices.pos = rotate(vertices.pos, cosAngle, sinAngle) - center;
            }
        }
    }
}

void Base::OnDraw()
{
}