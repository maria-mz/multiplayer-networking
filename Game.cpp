#include "Game.h"

Game::Game()
{
    m_window = nullptr;
    m_renderer = nullptr;
}

Game::~Game()
{
    // Destroy window
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    m_renderer = nullptr;
    m_window = nullptr;

    // Quit SDL subsystems
    SDL_Quit();

    m_network->shutdown();
}

bool Game::initWindow()
{
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LOG_ERROR("Couldn't initialize SDL: %s", SDL_GetError());
        success = false;
    }
    else
    {
        m_window = SDL_CreateWindow(Constants::WINDOW_TITLE,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   Constants::WINDOW_WIDTH,
                                   Constants::WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);

        if (m_window == nullptr)
        {
            LOG_ERROR("Couldn't create window: %s", SDL_GetError());
            success = false;
        }
    }

    if (TTF_Init() < 0)
    {
        LOG_ERROR("Couldn't initialize SDL TTF: %s", SDL_GetError());
        success = false;
    }

    return success;
}

bool Game::initRenderer()
{
    bool success = true;

    m_renderer = SDL_CreateRenderer(m_window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (m_renderer == nullptr)
    {
        LOG_ERROR("Couldn't create renderer: %s", SDL_GetError());
        success = false;
    }
    else
    {
        SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
        {
            LOG_ERROR("Couldn't initialize SDL_image: %s", SDL_GetError());
            success = false;
        }
    }

    return success;
}

bool Game::initTextures()
{
    bool success = true;

    if (
        !m_fontManager.loadFont(Constants::FILE_FONT_MAIN, 8) ||
        !m_fontManager.loadFont(Constants::FILE_FONT_MAIN, 12) ||
        !m_fontManager.loadFont(Constants::FILE_FONT_MAIN, 18)
    )
    {
        success = false;
    }

    return success;
}

bool Game::init()
{
    bool success = false;

    if (initWindow())
    {
        if (initRenderer())
        {
            if (initTextures())
            {
                success = true;
            }
        }
    }

    if (success)
    {
        m_player = std::unique_ptr<Player>(new Player());
        m_opponent = std::unique_ptr<Player>(new Player());

        m_network = std::shared_ptr<NetworkManager>(new NetworkManager());
    }

    return success;
}

void Game::spawnPlayers()
{
    m_player->m_transform.scale = 2;
    m_opponent->m_transform.scale = 2;

    if (m_isHost)
    {
        m_player->m_position.x = PLAYER_1_SPAWN_POSITION.x;
        m_player->m_position.y = PLAYER_1_SPAWN_POSITION.y;

        m_opponent->m_position.x = PLAYER_2_SPAWN_POSITION.x;
        m_opponent->m_position.y = PLAYER_2_SPAWN_POSITION.y;
    }
    else
    {
        m_player->m_position.x = PLAYER_2_SPAWN_POSITION.x;
        m_player->m_position.y = PLAYER_2_SPAWN_POSITION.y;

        m_opponent->m_position.x = PLAYER_1_SPAWN_POSITION.x;
        m_opponent->m_position.y = PLAYER_1_SPAWN_POSITION.y;
    }

    // Set initial net opponent data
    m_opponentNetcode.setNetPlayerData({
        {m_opponent->m_position.x, m_opponent->m_position.y},
        {m_opponent->m_velocity.x, m_opponent->m_velocity.y}
    });
}

void Game::handleOpponentNetMsgs()
{
    GameMessage opponentMsg;

    while (m_network->receiveOpponentMsg(opponentMsg))
    {
        switch (opponentMsg.type)
        {
            case GameMessageType::MovementUpdate:
            {
                m_opponentMovementUpdatesBuffer.push_back(opponentMsg.data.movementUpdate);
                break;
            }
        }
    }
}

void Game::sendPlayerMovementUpdate(const MovementUpdate &movementUpdate)
{
    GameMessage msg;
    msg.type = GameMessageType::MovementUpdate;
    msg.data.movementUpdate = movementUpdate;

    m_network->sendPlayerMsg(msg);
}

void Game::renderPlayer(std::shared_ptr<Player> player)
{
    player->render(m_renderer);
}

void Game::render()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);

    renderPlayer(m_player);
    renderPlayer(m_opponent);

    SDL_RenderPresent(m_renderer);
}

void Game::updateOpponent(int deltaTime)
{
    MovementUpdate movementUpdate;

    while (m_opponentMovementUpdatesBuffer.size() > MIN_OPPONENT_LAG_FRAMES)
    {
        movementUpdate = m_opponentMovementUpdatesBuffer.front();
        m_opponentMovementUpdatesBuffer.pop_front();

        if (movementUpdate.inputEvent != InputEvent::None)
        {
            m_opponent->input(movementUpdate.inputEvent);
        }

        if (movementUpdate.direction != m_opponent->m_direction)
        {
            m_opponent->m_direction = movementUpdate.direction;
        }

        m_opponentNetcode.updateNetState(movementUpdate);
    }

    m_opponent->update(deltaTime);
    m_opponentNetcode.syncPlayerWithNetState(*m_opponent);
}

void Game::handleEvent(const SDL_Event &event)
{
    InputEvent playerInputEvent = SDLEventTranslator::translate(event);
    if (playerInputEvent != InputEvent::None)
    {
        m_playerInputEvent = playerInputEvent;
        m_player->input(m_playerInputEvent);
    }
}

void Game::update(const int deltaTime)
{
    handleOpponentNetMsgs();

    m_player->update(deltaTime);
    updateOpponent(deltaTime);

    MovementUpdate playerMovementUpdate;
    playerMovementUpdate.posX = m_player->m_position.x;
    playerMovementUpdate.posY = m_player->m_position.y;
    playerMovementUpdate.velX = m_player->m_velocity.x;
    playerMovementUpdate.velY = m_player->m_velocity.y;
    playerMovementUpdate.inputEvent = m_playerInputEvent;
    playerMovementUpdate.direction = m_player->m_direction;

    sendPlayerMovementUpdate(playerMovementUpdate);
}

void Game::run()
{
    spawnPlayers();

    bool quit = false;
    SDL_Event event;
    FrameTimer frameTimer(GAME_TICK_RATE_MS);

    InputEvent playerInputEvent;

    while (!quit)
    {
        frameTimer.startFrame();

        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }

            handleEvent(event);
            break;
        }

        update(frameTimer.getDeltaTime());
        render();

        m_playerInputEvent = InputEvent::None;
        frameTimer.endFrame();
    }
}
