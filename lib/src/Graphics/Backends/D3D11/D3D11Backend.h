#ifndef __D3D11BACKEND_H_
#define __D3D11BACKEND_H_
#define __ENABLE_D3D11__

#if _WIN32 && defined(__ENABLE_D3D11__)
#ifndef _MSC_VER
#error "This complication only support MSVC compiler"
#endif

#include <Graphics/GraphicsBackendBase.h>
#include <d3d11.h>
#include <map>
#include <vector>

namespace Graphics {
    namespace Backends {
        struct D3D11Object
        {
            ID3D11Device        *dev;
            ID3D11DeviceContext *devcon;
            IDXGISwapChain      *swapchain;
        };

        class D3D11 : public Base
        {
        public:
            virtual ~D3D11() = default;

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

        private:
            void FlushQueue();

            D3D11Object Data;
        };
    } // namespace Backends
} // namespace Graphics

#endif
#endif