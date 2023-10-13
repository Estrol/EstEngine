#ifndef __Base_H_
#define __Base_H_

#include <Graphics/GraphicsBackendBase.h>
#include <Graphics/Utils/Rect.h>
#include <Math/Color3.h>
#include <Math/UDim2.h>
#include <Math/Vector2.h>

namespace UI {
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
        bool ClampToParent;

        Color3 Color3;
        Vector2 AnchorPoint;

        float Transparency;
        float Rotation;

        virtual void Draw();
        virtual void Draw(Rect clipRect);

        void CalculateSize();

      protected:
        virtual void OnDraw();

        Rect clipRect = {};
        void* image;
        Graphics::Backends::ShaderFragmentType shaderFragmentType;

        std::vector<Graphics::Backends::Vertex> m_vertices;
        std::vector<uint16_t> m_indices;

      private:
        void DrawVertices();
    };
} // namespace UI

#endif