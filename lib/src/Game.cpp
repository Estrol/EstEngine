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
        Graphics::API api = Graphics::API::OpenGL;

        auto window = Graphics::NativeWindow::Get();
        window->Init(title, width, height, api, fullscreen);

        auto renderer = Graphics::Renderer::Get();
        renderer->Init(api);

        auto engine = Audio::Engine::Get();
        engine->Init();

        auto inputs = Inputs::Manager::Get();
        window->AddSDLCallback([=](SDL_Event& event) {
            inputs->Update(event);
        });

        rect = std::make_unique<UI::Rectangle>();
        rect->Position = UDim2::fromOffset(50, 50);
        rect->Size = UDim2::fromOffset(150, 150);

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
            rect->Draw();
            renderer->EndFrame();
        }
    }

    return !window->ShouldExit();
}