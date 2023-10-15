#ifndef __NATIVE_WINDOW_H__
#define __NATIVE_WINDOW_H__

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include "Utils/Rect.h"
#include <SDL2/SDL.h>

namespace Graphics
{
    struct SDLWindowSmartDeallocator
    {
        inline void operator()(SDL_Window* window) const
        {
            SDL_DestroyWindow(window);
        }
    };

    enum class API;

    class NativeWindow
    {
    public:
        void Init(std::string title, int width, int height, API graphics, bool fullscreen);

        void PumpEvents();
        bool ShouldExit();

        Rect GetWindowSize();
        void SetWindowSize(Rect size);

        SDL_Window* GetWindow();

        void AddSDLCallback(std::function<void(SDL_Event&)> callback);

        static NativeWindow* Get();
        static void Destroy();

    private:
        ~NativeWindow();

        static NativeWindow* s_Instance;

        bool m_ShouldExit = false;
        Rect m_WindowRect;
        std::vector<std::function<void(SDL_Event&)>> m_Callbacks;
        std::unique_ptr<SDL_Window, SDLWindowSmartDeallocator> m_Window;
    };
}

#endif // __NATIVE_WINDOW_H__