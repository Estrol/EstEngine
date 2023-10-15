#ifndef __OPENGLBACKEND_H_
#define __OPENGLBACKEND_H_
#include <vector>
#include <map>
#include <Graphics/GraphicsBackendBase.h>

namespace Graphics {
    namespace Backends {
        struct ShaderData {
            uint32_t vert;
            uint32_t frag;
            uint32_t program;
        };

        struct OpenGLData {
            void* ctx;
            uint32_t vertexBuffer;
            uint32_t indexBuffer;
            uint32_t constantBuffer;
            uint32_t maxVertexBufferSize;
            uint32_t maxIndexBufferSize;

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

        private:
            void FlushQueue();
            OpenGLData Data;

            std::vector<SubmitInfo> submitInfos;
        };  
    } // namespace Backends
} // namespace Graphics

#endif