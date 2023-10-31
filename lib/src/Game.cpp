#include "Fonts/FontManager.h"
#include <Audio/AudioEngine.h>
#include <Exceptions/EstException.h>
#include <Game.h>
#include <Graphics/GraphicsBackendBase.h>
#include <Graphics/NativeWindow.h>
#include <Inputs/InputManager.h>
#include <MsgBox.h>
#include <Screens/ScreenManager.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>

Game::Game()
{
}

Game::~Game()
{
}

void Game::Run(RunInfo info)
{
    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result != 0) {
        std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return;
    }

    try {
        auto window = Graphics::NativeWindow::Get();
        window->Init(info.title, (int)info.resolution.X, (int)info.resolution.Y, info.graphics, info.fullscreen);

        auto engine = Audio::Engine::Get();
        engine->Init();

        auto inputs = Inputs::Manager::Get();
        window->AddSDLCallback([=](SDL_Event &event) {
            inputs->Update(event);
        });

        auto renderer = Graphics::Renderer::Get();
        auto scenemanager = Screens::Manager::Get();
        auto fontmanager = Fonts::FontManager::Get();

        if (info.threadMode == ThreadMode::Multi) {
            auto oninit = [=]() {
                scenemanager->Init(this);
                renderer->Init(info.graphics, info.samplerInfo);
                OnLoad();
            };

            auto onshutdown = [=]() {
                OnUnload();
                Screens::Manager::Destroy();
                Fonts::FontManager::Destroy();
                Graphics::Renderer::Destroy();
            };

            auto onupdate = [=](double delta) {
                OnUpdate(delta);

                bool shouldDraw = renderer->BeginFrame();
                if (shouldDraw) {
                    OnDraw(delta);
                    renderer->EndFrame();
                }
            };

            m_DrawThread = Thread(oninit, onupdate, onshutdown, 240.0);

            auto oninput = [=](double delta) {
                window->PumpEvents();

                OnInput(delta);
            };

            m_InputThread = Thread(oninput, 1000.0);

            m_DrawThread.Start();

            while (!window->ShouldExit()) {
                m_InputThread.Tick();
            }

            m_DrawThread.Stop();
        } else {
            renderer->Init(info.graphics, info.samplerInfo);
            scenemanager->Init(this);
            OnLoad();

            auto oninput = [=](double delta) {
                window->PumpEvents();

                OnInput(delta);
                OnUpdate(delta);

                bool shouldDraw = renderer->BeginFrame();
                if (shouldDraw) {
                    OnDraw(delta);
                    renderer->EndFrame();
                }
            };

            m_InputThread = Thread(oninput, 1000.0);

            while (!window->ShouldExit()) {
                m_InputThread.Tick();
            }

            OnUnload();

            Screens::Manager::Destroy();
            Fonts::FontManager::Destroy();
            Graphics::Renderer::Destroy();
        }

        Graphics::NativeWindow::Destroy();
        Inputs::Manager::Destroy();

    } catch (Exceptions::EstException &e) {
        MsgBox::Show("Error", e.what(), MsgBox::Type::Ok, MsgBox::Flags::Error);
    }

    SDL_Quit();
}

void Game::OnLoad()
{
    rect = std::make_unique<UI::Text>();
}

void Game::OnUnload()
{
    rect.reset();
}

void Game::OnInput(double delta)
{
    auto scenemanager = Screens::Manager::Get();
    scenemanager->Input(delta);
}

void Game::OnUpdate(double delta)
{
    auto scenemanager = Screens::Manager::Get();
    scenemanager->Update(delta);
}

void Game::OnDraw(double delta)
{
    auto scenemanager = Screens::Manager::Get();
    scenemanager->Draw(delta);
}