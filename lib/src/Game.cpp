#include <Game.h>
#include <iostream>
#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#include <Graphics/GraphicsBackendBase.h>
#include <Graphics/Renderer.h>
#include <Audio/AudioEngine.h>
#include <Inputs/Manager.h>
#include <MsgBox.h>
#include <SDL2/SDL.h>

Game::Game()
{

}

Game::~Game()
{

}

void Game::Run(std::string title, int width, int height, bool fullscreen)
{
    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result != 0)
    {
        std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }

    try {
        auto window = Graphics::NativeWindow::Get();
        window->Init(title, width, height, fullscreen);

        auto renderer = Graphics::Renderer::Get();
        renderer->Init(Graphics::API::Vulkan);

        auto engine = Audio::Engine::Get();
        engine->Init();

        auto inputs = Inputs::Manager::Get();
        window->AddSDLCallback([=](SDL_Event& event) {
            inputs->Update(event);
        });

        while(Tick()) {
            
        }
        
    } catch (Exceptions::EstException& e) {
        MsgBox::Show("Error", e.what(), MsgBox::Type::Ok, MsgBox::Flags::Error);
    }

    Graphics::Renderer::Destroy();
    Graphics::NativeWindow::Destroy();
    Inputs::Manager::Destroy();
}

bool Game::Tick()
{
    auto window = Graphics::NativeWindow::Get();
    window->PumpEvents();

    auto renderer = Graphics::Renderer::Get();

    {
        auto doRender = renderer->BeginFrame();
        if (doRender) {
            renderer->EndFrame();
        }
    }

    return !window->ShouldExit();
}