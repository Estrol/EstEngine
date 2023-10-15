#ifndef __GRAPHICSBACKENDBASE_H_
#define __GRAPHICSBACKENDBASE_H_

#include <glm/glm.hpp>
#include <vector>
#include "Utils/Rect.h"

#define MY_OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

namespace Graphics {
    namespace Backends {
        struct Vertex {
            glm::vec2 pos;
            glm::vec2 texCoord;
            uint32_t color;

            inline void SetColorFloat(glm::vec4 color) {
                auto r = color.r;
                auto g = color.g;
                auto b = color.b;
                auto a = color.a;

                this->color = ((uint32_t)(a * 255.0f) << 24) | ((uint32_t)(b * 255.0f) << 16) | ((uint32_t)(g * 255.0f) << 8) | ((uint32_t)(r * 255.0f) << 0);
            };

            inline void SetColorRGB(glm::vec4 color) {
                auto r = color.r; // they are in range 0-255
                auto g = color.g;
                auto b = color.b;
                auto a = color.a;

                this->color = ((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | ((uint32_t)(r) << 0);
            };

            // PRIVATE
            glm::vec2 scale;
            glm::vec2 translate;
        };

        enum class ShaderFragmentType {
            Solid,
            Image
        };

        struct SubmitInfo {
            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;

            Rect clipRect;

            uint32_t image = NULL;
            ShaderFragmentType fragmentType;
        };

        class Base
        {
        public:
            virtual ~Base() = default;

            virtual void Init() = 0;
            virtual void ReInit() = 0;
            virtual void Shutdown() = 0;

            virtual bool NeedReinit() = 0;

            virtual bool BeginFrame() = 0;
            virtual void EndFrame() = 0;

            virtual void ImGui_Init() = 0;
            virtual void ImGui_DeInit() = 0;
            virtual void ImGui_NewFrame() = 0;
            virtual void ImGui_EndFrame() = 0;

            virtual void Push(SubmitInfo& info) = 0;
        };
    }
}

#endif