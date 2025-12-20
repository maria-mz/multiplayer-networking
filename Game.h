#ifndef GAME_H_
#define GAME_H_

#include <iostream>
#include <stdio.h>
#include <deque>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "Logging.h"
#include "Player.h"
#include "Resources.h"
#include "NetworkManager.h"
#include "Netcode.h"
#include "FrameTimer.h"
#include "Utils/Utils.h"
#include "UI/MenuUI.h"

constexpr const int GAME_TICK_RATE_MS = 16; // ~16 ms per frame (~60 updates per second)

const Vector2D PLAYER_1_SPAWN_POSITION(180, 245);
const Vector2D PLAYER_2_SPAWN_POSITION(380, 245);

constexpr const int PLAYER_1_ID = 1;
constexpr const int PLAYER_2_ID = 2;

constexpr const int MIN_OPPONENT_LAG_FRAMES = 1;

class Game
{
    public:
        Game();

        bool init();
        void run();

        ~Game();

    private:
        bool initWindow();
        bool initRenderer();
        bool initTextures();

        void spawnPlayers();

        void handleOpponentNetMsgs();
        void updateOpponent(int deltaTime);

        void sendPlayerMovementUpdate(const MovementUpdate &movementUpdate);

        void handleEvent(const SDL_Event &event);
        void update(const int deltaTime);

        void render();
        void renderGameplay();
        void renderPlayer(std::shared_ptr<Player> player);

        SDL_Window *m_window;
        SDL_Renderer *m_renderer;

        std::shared_ptr<Player> m_player;
        std::shared_ptr<Player> m_opponent;

        InputEvent m_playerInputEvent;

        bool m_isHost;

        std::shared_ptr<NetworkManager> m_network;
        Netcode m_opponentNetcode;
        std::deque<MovementUpdate> m_opponentMovementUpdatesBuffer;
};

#endif