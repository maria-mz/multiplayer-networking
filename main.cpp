#include "Game.h"
#include "RenderSystem.h"
#include "FrameTimer.h"

int main(int argc, char* args[])
{
    RenderSystem renderSystem;
    Game game;

    if (!renderSystem.init())
    {
        return 1;
    }

    bool quit = false;
    FrameTimer frameTimer(GAME_TICK_RATE_MS);

    std::vector<GameMessage> inMessages{};

    SDL_Event event;
    InputEvent inputEvent;

    while (!quit)
    {
        frameTimer.startFrame();

        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
            inputEvent = SDLEventTranslator::translate(event);
            break;
        }

        game.input(inputEvent);
        game.update(frameTimer.getDeltaTime(), inMessages);

        renderSystem.renderGame(game);

        frameTimer.endFrame();

        inputEvent = InputEvent::None;
    }

    return 0;
}
