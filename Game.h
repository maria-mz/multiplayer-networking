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
#include "NetworkManager.h"
#include "Netcode.h"
#include "FrameTimer.h"
#include "Utils/Utils.h"
#include "UI/MenuUI.h"

constexpr const int GAME_TICK_RATE_MS = 16; // ~16 ms per frame (~60 updates per second)

const Vector2D INITIAL_SPAWN_POSITION(0, 0);

constexpr const int MIN_OPPONENT_LAG_FRAMES = 1;

class Game;

class RenderSystem
{
    public:
        bool init();

        void renderGame(Game& game);
        void renderPlayer(Player& player);

    private:
        bool initWindow();
        bool initRenderer();
        bool initFonts();

        SDL_Window *m_window;
        SDL_Renderer *m_renderer;
        FontManager m_fontManager;
};

class Game
{
    public:
        Game();

        void run();

        ~Game();

    // private:
        void handleOpponentNetMsgs();
        void updateOpponent(int deltaTime);

        void sendPlayerMovementUpdate(const MovementUpdate &movementUpdate);

        void handleEvent(const SDL_Event &event);
        void update(const int deltaTime);

        InputEvent m_lastPlayerInputEvent;

        bool m_isHost;
        int m_playerIDCounter = 1;

        std::shared_ptr<NetworkManager> m_network;
        // Netcode m_opponentNetcode;
        // std::deque<MovementUpdate> m_opponentMovementUpdatesBuffer;

        std::unique_ptr<Player> m_thisPlayer;
};

#endif