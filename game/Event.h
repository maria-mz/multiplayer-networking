#pragma once

#include <variant>
#include <SDL2/SDL.h>

enum class AppEvent
{
    None,
    Quit
};

enum class GameEvent {
    None,
    MoveLeft_Pressed,
    MoveRight_Pressed,
    MoveLeft_Released,
    MoveRight_Released,
};

using Event = std::variant<std::monostate, AppEvent, GameEvent>;


class SDLEventTranslator
{
    public:
        static Event translate(const SDL_Event &SDLEvent)
        {
            Event event{};

            if (SDLEvent.type == SDL_KEYDOWN && SDLEvent.key.repeat == 0)
            {
                switch (SDLEvent.key.keysym.sym)
                {
                    case SDLK_d: event = GameEvent::MoveRight_Pressed; break;
                    case SDLK_a: event = GameEvent::MoveLeft_Pressed; break;
                }
            }

            else if (SDLEvent.type == SDL_KEYUP && SDLEvent.key.repeat == 0)
            {
                switch (SDLEvent.key.keysym.sym)
                {
                    case SDLK_d: event = GameEvent::MoveRight_Released; break;
                    case SDLK_a: event = GameEvent::MoveLeft_Released; break;
                }
            }

            else if (SDLEvent.type == SDL_QUIT)
            {
                event = AppEvent::Quit;
            }

            return event;
        }
};
