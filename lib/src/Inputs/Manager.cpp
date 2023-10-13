#include <Inputs/Manager.h>
using namespace Inputs;

Manager* Manager::Instance = nullptr;

Manager::Manager()
{
    OnKeyEvent = []() {};
    OnMouseEvent = []() {};
}

Manager::~Manager()
{
}

Manager* Manager::Get()
{
    if (Instance == nullptr) {
        Instance = new Manager();
    }

    return Instance;
}

void Manager::Destroy()
{
    if (Instance != nullptr) {
        delete Instance;
        Instance = nullptr;
    }
}

void Manager::Update(SDL_Event& event)
{
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        HandleKeyEvent(event);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        HandleMouseEvent(event);
        break;
    case SDL_MOUSEMOTION:
        HandleMouseMotionEvent(event);
        break;
    }
}

void Manager::HandleKeyEvent(SDL_Event& event)
{
    bool previus = KeyStates[(Keys)event.key.keysym.scancode];

    switch (event.type) {
    case SDL_KEYDOWN:
        KeyStates[(Keys)event.key.keysym.scancode] = true;
        break;
    case SDL_KEYUP:
        KeyStates[(Keys)event.key.keysym.scancode] = false;
        break;
    }

    if (previus != KeyStates[(Keys)event.key.keysym.scancode]) {
        OnKeyEvent();
    }
}

void Manager::HandleMouseEvent(SDL_Event& event)
{
    bool previus = MouseStates[(Mouse)event.button.button];

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        MouseStates[(Mouse)event.button.button] = true;
        break;
    case SDL_MOUSEBUTTONUP:
        MouseStates[(Mouse)event.button.button] = false;
        break;
    }

    if (previus != MouseStates[(Mouse)event.button.button]) {
        OnMouseEvent();
    }
}

void Manager::HandleMouseMotionEvent(SDL_Event& event)
{
    MousePosition.X = event.motion.x;
    MousePosition.Y = event.motion.y;
}

bool Manager::IsKeyDown(Keys key)
{
    return KeyStates[key];
}

bool Manager::IsMouseDown(Mouse button)
{
    return MouseStates[button];
}

void Manager::ListenOnKeyEvent(std::function<void()> callback)
{
    OnKeyEvent = callback;
}

void Manager::ListenOnMouseEvent(std::function<void()> callback)
{
    OnMouseEvent = callback;
}