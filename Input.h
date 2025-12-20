#ifndef INPUT_H_
#define INPUT_H_

#include <SDL2/SDL.h>

enum class Input {
    MoveLeft,
    MoveRight,
};

enum class InputEvent {
    None,
    MoveLeft_Pressed,
    MoveRight_Pressed,
    MoveLeft_Released,
    MoveRight_Released,
};

class SDLEventTranslator
{
    public:
        static InputEvent translate(const SDL_Event &SDLEvent)
        {
            InputEvent inputEvent = InputEvent::None;

            if (SDLEvent.type == SDL_KEYDOWN && SDLEvent.key.repeat == 0)
            {
                switch (SDLEvent.key.keysym.sym)
                {
                    case SDLK_d: inputEvent = InputEvent::MoveRight_Pressed; break;
                    case SDLK_a: inputEvent = InputEvent::MoveLeft_Pressed; break;
                }
            }

            else if (SDLEvent.type == SDL_KEYUP && SDLEvent.key.repeat == 0)
            {
                switch (SDLEvent.key.keysym.sym)
                {
                    case SDLK_d: inputEvent = InputEvent::MoveRight_Released; break;
                    case SDLK_a: inputEvent = InputEvent::MoveLeft_Released; break;
                }
            }

            return inputEvent;
        }
};

#endif