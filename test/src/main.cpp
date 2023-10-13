#include <Game.h>
#include <Audio/AudioEngine.h>
#include <Audio/AudioStream.h>

int main()
{
    Game game;
    game.Run("Test", 1280, 720, false);
}