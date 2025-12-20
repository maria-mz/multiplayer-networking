#pragma once

#include <cassert>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "Logging.h"
#include "Player.h"
#include "Game.h"
#include "FontManager.h"


class RenderSystem
{
    public:
        RenderSystem()
        {
            m_window = nullptr;
            m_renderer = nullptr;
        }

        ~RenderSystem()
        {
            // Destroy window
            SDL_DestroyRenderer(m_renderer);
            SDL_DestroyWindow(m_window);
            m_renderer = nullptr;
            m_window = nullptr;

            // Quit SDL subsystems
            SDL_Quit();
        }

        bool init()
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

        void renderGame(Game& game)
        {
            assert(m_renderer != nullptr);

            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderClear(m_renderer);

            renderPlayer(game.m_player);

            SDL_RenderPresent(m_renderer);
        }

        void renderPlayer(Player& player)
        {
            assert(m_renderer != nullptr);
            player.render(m_renderer);
        }

    private:
        bool initWindow()
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

        bool initRenderer()
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

        bool initFonts()
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

        SDL_Window *m_window;
        SDL_Renderer *m_renderer;
        FontManager m_fontManager;
};
