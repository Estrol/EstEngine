#include "UIBase.h"
#include <Graphics/NativeWindow.h>
#include <Graphics/Renderer.h>
#include <algorithm>

using namespace UI;

Base::Base()
{
    Position = UDim2::fromScale(0.0, 0.0);
    Size = UDim2::fromScale(1.0, 1.0);
    AnchorPoint = { 0.0f, 0.0f };
    Color3 = Color3::FromRGB(255, 255, 255);
    Transparency = 1;
    Rotation = 0;
    Parent = nullptr;
    ClampToParent = false;
}

Base::~Base()
{
}

void Base::Draw()
{
    clipRect = Graphics::NativeWindow::Get()->GetWindowSize();
    DrawVertices();
}

void Base::Draw(Rect _clipRect)
{
    clipRect = _clipRect;
    DrawVertices();
}

void Base::CalculateSize()
{
    auto windowRect = Graphics::NativeWindow::Get()->GetWindowSize();
    float Width = windowRect.Width;
    float Height = windowRect.Height;
    float X = 0;
    float Y = 0;

    if (Parent != nullptr && Parent != this) {
        Parent->CalculateSize();

        Width = (float)Parent->AbsoluteSize.X;
        Height = (float)Parent->AbsoluteSize.Y;
        X = (float)Parent->AbsolutePosition.X;
        Y = (float)Parent->AbsolutePosition.Y;
    }

    float x0 = Width * Position.X.Scale + Position.X.Offset;
    float y0 = Height * Position.Y.Scale + Position.Y.Offset;

    float x1 = Width * Size.X.Scale + Size.X.Offset;
    float y1 = Height * Size.Y.Scale + Size.Y.Offset;

    float xAnchor = x1 * std::clamp((float)AnchorPoint.X, 0.0f, 1.0f);
    float yAnchor = y1 * std::clamp((float)AnchorPoint.Y, 0.0f, 1.0f);

    x0 -= xAnchor;
    y0 -= yAnchor;

    AbsolutePosition = { X + x0, Y + y0 };
    AbsoluteSize = { x1, y1 };
}

void Base::DrawVertices()
{
    using namespace Graphics::Backends;
    auto renderer = Graphics::Renderer::Get();

    SubmitInfo info = {};
    info.clipRect = clipRect;
    info.vertices = m_vertices;
    info.fragmentType = shaderFragmentType;
    info.indices = m_indices;

    renderer->Push(info);
}