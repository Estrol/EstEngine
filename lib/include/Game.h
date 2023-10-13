#ifndef __GAME_H__
#define __GAME_H__
#include <string>

class Game
{
public:
    Game();
    virtual ~Game();
    void Run(std::string title, int width, int height, bool fullscreen);

private:
    bool Tick();
};

#endif // __GAME_H__