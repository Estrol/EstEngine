#include "./Backends/Vulkan/VulkanBackend.h"
#include <Exceptions/EstException.h>
#include <Graphics/Renderer.h>
#include <iostream>
using namespace Graphics;

Renderer *Renderer::s_Instance = nullptr;

Renderer *Renderer::Get()
{
    if (s_Instance == nullptr) {
        s_Instance = new Renderer();
    }
    return s_Instance;
}

void Renderer::Destroy()
{
    if (s_Instance != nullptr) {
        delete s_Instance;
        s_Instance = nullptr;
    }
}

void Renderer::Init(API api)
{
    using namespace Backends;

    m_API = api;

    Base *backend = nullptr;
    switch (api) {
        case API::Vulkan:
        {
            backend = new Vulkan();
            break;
        }

        case API::OpenGL:
        {
            throw Exceptions::EstException("OpenGL is not supported yet");
        }

        default:
        {
            throw Exceptions::EstException("Unknown API");
        }
    }

    backend->Init();

    m_Backend = backend;
}

Backends::Base *Renderer::GetBackend()
{
    return m_Backend;
}

API Renderer::GetAPI()
{
    return m_API;
}

void Renderer::Push(Graphics::Backends::SubmitInfo& info) 
{
    m_Backend->Push(info);
}

bool Renderer::BeginFrame()
{
    if (!m_Backend) {
        throw Exceptions::EstException("Renderer backend not initialized");
    }

    if (m_onFrame) {
        throw Exceptions::EstException("BeginFrame called without EndFrame");
    }

    if (m_Backend->NeedReinit()) {
        m_Backend->ReInit();
    }

    auto result = m_Backend->BeginFrame();
    m_onFrame = result;
    return result;
}

void Renderer::EndFrame()
{
    if (!m_Backend) {
        throw Exceptions::EstException("Renderer backend not initialized");
    }

    if (!m_onFrame) {
        throw Exceptions::EstException("EndFrame called without BeginFrame");
    }

    m_onFrame = false;
    m_Backend->EndFrame();
}