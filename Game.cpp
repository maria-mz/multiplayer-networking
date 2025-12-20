#include "Game.h"
#include <cassert>

RenderSystem::RenderSystem()
{
    m_window = nullptr;
    m_renderer = nullptr;
}

RenderSystem::~RenderSystem()
{
    // Destroy window
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    m_renderer = nullptr;
    m_window = nullptr;

    // Quit SDL subsystems
    SDL_Quit();
}

bool RenderSystem::initWindow()
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

bool RenderSystem::initRenderer()
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

bool RenderSystem::initFonts()
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

bool RenderSystem::init()
{
    bool success = false;

    if (initWindow())
    {
        if (initRenderer())
        {
            if (initFonts())
            {
                success = true;
            }
        }
    }

    return success;
}

void RenderSystem::renderGame(Game& game)
{
    assert(m_renderer != nullptr);

    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);

    renderPlayer(*game.m_thisPlayer);

    SDL_RenderPresent(m_renderer);
}

void RenderSystem::renderPlayer(Player& player)
{
    assert(m_renderer != nullptr);
    player.render(m_renderer);
}


Game::Game()
{

}

Game::~Game()
{
    m_network->shutdown();
}

void Game::handleOpponentNetMsgs()
{
    GameMessage opponentMsg;

    while (m_network->receiveOpponentMsg(opponentMsg))
    {
        // switch (opponentMsg.type)
        // {
        //     case GameMessageType::MovementUpdate:
        //     {
        //         m_opponentMovementUpdatesBuffer.push_back(opponentMsg.data.movementUpdate);
        //         break;
        //     }
        // }
    }
}

void Game::sendPlayerMovementUpdate(const MovementUpdate &movementUpdate)
{
    GameMessage msg;
    msg.type = GameMessageType::MovementUpdate;
    msg.data.movementUpdate = movementUpdate;

    m_network->sendPlayerMsg(msg);
}

void Game::updateOpponent(int deltaTime)
{
    // MovementUpdate movementUpdate;

    // while (m_opponentMovementUpdatesBuffer.size() > MIN_OPPONENT_LAG_FRAMES)
    // {
    //     movementUpdate = m_opponentMovementUpdatesBuffer.front();
    //     m_opponentMovementUpdatesBuffer.pop_front();

    //     if (movementUpdate.inputEvent != InputEvent::None)
    //     {
    //         m_opponent->input(movementUpdate.inputEvent);
    //     }

    //     if (movementUpdate.direction != m_opponent->m_direction)
    //     {
    //         m_opponent->m_direction = movementUpdate.direction;
    //     }

    //     m_opponentNetcode.updateNetState(movementUpdate);
    // }

    // m_opponent->update(deltaTime);
    // m_opponentNetcode.syncPlayerWithNetState(*m_opponent);
}

void Game::handleEvent(const SDL_Event &event)
{
    InputEvent playerInputEvent = SDLEventTranslator::translate(event);
    if (playerInputEvent != InputEvent::None)
    {
        m_lastPlayerInputEvent = playerInputEvent;
        m_thisPlayer->input(m_lastPlayerInputEvent);
    }
}

void Game::update(const int deltaTime)
{
    handleOpponentNetMsgs();

    m_thisPlayer->update(deltaTime);
    updateOpponent(deltaTime);

    MovementUpdate playerMovementUpdate;
    playerMovementUpdate.posX = m_thisPlayer->m_position.x;
    playerMovementUpdate.posY = m_thisPlayer->m_position.y;
    playerMovementUpdate.velX = m_thisPlayer->m_velocity.x;
    playerMovementUpdate.velY = m_thisPlayer->m_velocity.y;
    playerMovementUpdate.inputEvent = m_lastPlayerInputEvent;
    playerMovementUpdate.direction = m_thisPlayer->m_direction;

    sendPlayerMovementUpdate(playerMovementUpdate);
}

void Game::run()
{
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
        // render();

        m_lastPlayerInputEvent = InputEvent::None;
        frameTimer.endFrame();
    }
}
