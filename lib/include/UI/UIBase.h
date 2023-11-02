#ifndef __Base_H_
#define __Base_H_

#include <Graphics/GraphicsBackendBase.h>
#include <Graphics/GraphicsTexture2D.h>
#include <Graphics/Utils/Rect.h>
#include <Math/Color3.h>
#include <Math/UDim2.h>
#include <Math/Vector2.h>
#include <Math/Vector4.h>

namespace UI {
    enum class RenderMode {
        Normal,
        Batches,
    };

    class Base
    {
    public:
        Base();
        virtual ~Base();

        Vector2 AbsolutePosition;
        Vector2 AbsoluteSize;

        UDim2 Position;
        UDim2 Size;

        Base *Parent;
        bool  ClampToParent;

        Color3  Color3;
        Vector2 AnchorPoint;
        Vector4 CornerRadius;

        Graphics::Backends::BlendHandle BlendState;

        float Transparency;
        float Rotation;

        virtual void Draw();
        virtual void Draw(Rect clipRect);

        void CalculateSize();

    protected:
        virtual void OnDraw();
        void         InsertToBatch();
        void         RotateVertex();

        glm::vec4 roundedCornerPixels;

        Rect                                   clipRect = {};
        Graphics::Backends::ShaderFragmentType shaderFragmentType;

        std::vector<Graphics::Backends::Vertex> m_vertices;
        std::vector<uint16_t>                   m_indices;

        std::unique_ptr<Graphics::Texture2D> m_texture;
        Graphics::Texture2D                 *m_texturePtr = nullptr;
        RenderMode                           m_renderMode = RenderMode::Normal;

        std::vector<Graphics::Backends::SubmitInfo> m_batches;

    private:
        void DrawVertices();
    };
} // namespace UI

#endif