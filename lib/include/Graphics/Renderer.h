#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <string>
#include "GraphicsBackendBase.h"

namespace Graphics 
{
    enum class API
    {
        None = 0,
        OpenGL = 1,
        Vulkan = 2,
    };

    class Renderer
    {
    public:
        void Init(API api);

        bool BeginFrame();
        void EndFrame();

        void Push(Graphics::Backends::SubmitInfo& info);

        Backends::Base* GetBackend();
        API GetAPI();

        static Renderer* Get();
        static void Destroy();
    private:
        static Renderer* s_Instance;

        API m_API;
        Backends::Base* m_Backend;

        bool m_onFrame = false;
    };
}

#endif // __RENDERER_H__