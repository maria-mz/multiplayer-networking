#include "Game.h"
#include "FrameTimer.h"

int main(int argc, char* args[])
{
    Game game = Game();

    if (game.init())
    {
        game.run();
    }
}
