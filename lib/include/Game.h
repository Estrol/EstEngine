#ifndef __GAME_H__
#define __GAME_H__
#include <string>
#include <memory>
#include "UI/Rectangle.h"

class Game
{
public:
    Game();
    virtual ~Game();
    void Run(std::string title, int width, int height, bool fullscreen);

private:
    bool Tick();

    std::unique_ptr<UI::Rectangle> rect;
};

#endif // __GAME_H__