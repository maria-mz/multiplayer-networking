#ifndef GAME_H_
#define GAME_H_

#include <iostream>
#include <stdio.h>
#include <deque>
#include <unordered_map>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "Logging.h"
#include "Player.h"
#include "GameMessage.h"


constexpr const int GAME_TICK_RATE_MS = 16; // ~16 ms per frame (~60 updates per second)

const Vector2D INITIAL_SPAWN_POSITION(0, 0);

constexpr const int MIN_OPPONENT_LAG_FRAMES = 1;


class Game
{
    public:
        void input(InputEvent event);
        std::vector<GameMessage> update(const int deltaTime,
                                        std::vector<GameMessage>& inMessages);

        bool m_isHost;
        int m_playerIDCounter = 1;

        int m_playerID = 0;
        Player m_player;

    private:
        InputEvent m_lastInputEvent;
};

#endif