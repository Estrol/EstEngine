#include "D3D11Backend.h"

#if defined(__ENABLE_D3D11__)

#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#include <SDl2/SDL_syswm.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")

using namespace Graphics::Backends;

#define ESTZEROMEMORY(data, len) \
    memset((void *)&data, 0, len)

#define CHECKERROR(hr, err_msg)                  \
    if (!SUCCEEDED(hr)) {                        \
        throw Exceptions::EstException(err_msg); \
    }

D3D11::~D3D11()
{
}

void D3D11::Init()
{
    SDL_Window *wnd = Graphics::NativeWindow::Get()->GetWindow();

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    if (!SDL_GetWindowWMInfo(wnd, &info)) {
        throw Exceptions::EstException("Failed to init D3D11: Failed to get window context");
    }

    HWND hwnd = info.info.win.window;

    DXGI_SWAP_CHAIN_DESC scd;
    ESTZEROMEMORY(scd, sizeof(scd));

    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 4;
    scd.Windowed = TRUE;

    auto HR = D3D11CreateDeviceAndSwapChain(NULL,
                                            D3D_DRIVER_TYPE_HARDWARE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            D3D11_SDK_VERSION,
                                            &scd,
                                            &Data.swapchain,
                                            &Data.dev,
                                            NULL,
                                            &Data.devcon);

    CHECKERROR(HR, "Failed to create device and swap chain");
}

void D3D11::ReInit() {}

bool D3D11::NeedReinit() {}

bool D3D11::BeginFrame() {}

void D3D11::EndFrame() {}

#endif