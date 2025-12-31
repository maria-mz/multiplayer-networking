#pragma once

#include "SDL2/SDL.h"
#include "event.h"

#include <vector>

class InputSystem
{
    public:
        std::vector<Event> poll()
        {
            std::vector<Event> events;
            SDL_Event SDLEvent;
            Event event;

            while (SDL_PollEvent(&SDLEvent) != 0)
            {
                event = SDLEventTranslator::translate(SDLEvent);
                if (!std::holds_alternative<std::monostate>(event))
                {
                    events.push_back(event);
                }
            }
            return events;
        }
};
