#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#include <Graphics/Renderer.h>

using namespace Graphics;

NativeWindow *NativeWindow::s_Instance = nullptr;

NativeWindow *NativeWindow::Get()
{
    if (s_Instance == nullptr) {
        s_Instance = new NativeWindow();
    }
    return s_Instance;
}

void NativeWindow::Destroy()
{
    if (s_Instance != nullptr) {
        delete s_Instance;
        s_Instance = nullptr;
    }
}

void NativeWindow::Init(std::string title, int width, int height, API graphics, bool fullscreen)
{
    Uint32 flags = 0;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    switch (graphics) {
        case API::OpenGL:
        {
            flags |= SDL_WINDOW_OPENGL;
            break;
        }

        case API::Vulkan:
        {
            flags |= SDL_WINDOW_VULKAN;
            break;
        }

        default:
        {
            throw Exceptions::EstException("Unknown graphics API");
        }
    }

    auto Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
    if (Window == nullptr) {
        throw Exceptions::EstException("Failed to initialize window");
    }

    m_Window = std::unique_ptr<SDL_Window, SDLWindowSmartDeallocator>(Window);
    m_WindowRect = { 0, 0, width, height };
}

NativeWindow::~NativeWindow()
{
}

Rect NativeWindow::GetWindowSize()
{
    Rect rc = {};
    SDL_GetWindowSize(m_Window.get(), &rc.Width, &rc.Height);

    if (SDL_GetWindowFlags(m_Window.get()) & SDL_WINDOW_MINIMIZED) {
        rc.Width = 0;
        rc.Height = 0;
    }

    return rc;
}

void NativeWindow::SetWindowSize(Rect size)
{
    SDL_SetWindowSize(m_Window.get(), size.Width, size.Height);
    m_WindowRect = size;
}

SDL_Window *NativeWindow::GetWindow()
{
    return m_Window.get();
}

void NativeWindow::PumpEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_ShouldExit = true;
                break;
        }

        for (auto &callback : m_Callbacks) {
            callback(event);
        }
    }
}

void NativeWindow::AddSDLCallback(std::function<void(SDL_Event &)> callback)
{
    m_Callbacks.push_back(callback);
}

bool NativeWindow::ShouldExit()
{
    return m_ShouldExit;
}