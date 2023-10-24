#ifndef __GAME_H__
#define __GAME_H__
#include "Threads/Thread.h"
#include "UI/Text.h"
#include <Graphics/NativeWindow.h>
#include <Graphics/Renderer.h>
#include <Math/Vector2.h>
#include <memory>
#include <string>

enum class ThreadMode {
    Single,
    Multi
};

struct RunInfo
{
    std::string                  title;
    Vector2                      resolution;
    bool                         fullscreen;
    ThreadMode                   threadMode;
    Graphics::TextureSamplerInfo samplerInfo;
    Graphics::API                graphics;
};

class Game
{
public:
    Game();
    virtual ~Game();

    void Run(RunInfo info);

protected:
    /**
     * Called when the game is initialized
     * Used for loading resources, always use this function to initialize resources to avoid threading problems with the renderer
     * Thread: Render
     */
    virtual void OnLoad();

    /**
     * Called when the game is deinitialized
     * Used for unloading resources, always use this function
     * Thread: Render
     */
    virtual void OnUnload();

    /*
     * Called when the game should update the input
     * You might want to use this to update the input state like keyboard and mouse
     * Thread: Window
     */
    virtual void OnInput(double delta);

    /**
     * Called when the game should update
     * Thread: Render
     */
    virtual void OnUpdate(double delta);

    /**
     * Called when the game should draw
     * it might not be called every frame like window minimized
     * Thread: Render
     */
    virtual void OnDraw(double delta);

private:
    Thread m_InputThread;
    Thread m_DrawThread;

    std::unique_ptr<UI::Text> rect;
};

#endif // __GAME_H__