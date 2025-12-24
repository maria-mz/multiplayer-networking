#pragma once

#include <cassert>
#include <format>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "../common/Logging.h"
#include "../ui/Text.h"
#include "Player.h"
#include "FontManager.h"

#include "GameSimulation.h"


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

            if (success)
            {
                m_playerLabelAttributes.font = m_fontManager.getFont(
                    Constants::FILE_FONT_MAIN.c_str(), 12
                );
                m_playerLabelAttributes.color = {0, 0, 0, 255};
                m_playerLabelAttributes.outlinePx = 0;
            }

            return success;
        }

        void renderGame(GameSimulation& gameSimulation)
        {
            assert(m_renderer != nullptr);

            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderClear(m_renderer);

            for (auto& [id, player] : gameSimulation.getPlayers())
            {
                renderPlayer(*player, id);
            }

            SDL_RenderPresent(m_renderer);
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
                !m_fontManager.loadFont(Constants::FILE_FONT_MAIN.c_str(), 8) ||
                !m_fontManager.loadFont(Constants::FILE_FONT_MAIN.c_str(), 12) ||
                !m_fontManager.loadFont(Constants::FILE_FONT_MAIN.c_str(), 18)
            )
            {
                success = false;
            }

            return success;
        }

        void renderPlayer(Player& player, PlayerID playerID)
        {
            assert(m_renderer != nullptr);

            std::string idText = std::format("ID: {}", playerID);
            std::string positionText = std::format(
                "({:.0f}, {:.0f})",
                player.m_position.x,
                player.m_position.y
            );

            Text idLabel = Text(m_playerLabelAttributes, idText, m_renderer);
            Text positionLabel = Text(m_playerLabelAttributes, positionText, m_renderer);

            // Draw player
            SDL_Rect box = player.getBoundingBox();

            // Draw fill
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 255, 64);
            SDL_RenderFillRect(m_renderer, &box);

            // Draw border
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 150, 255);
            SDL_RenderDrawRect(m_renderer, &box);
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);

            // Draw label
            int centerX = box.x + box.w / 2;

            int idX = centerX - idLabel.getWidth() / 2;
            int posX = centerX - positionLabel.getWidth() / 2;

            int topY = box.y - positionLabel.getHeight() - idLabel.getHeight();
            bool drawAbove = topY >= 0;

            int idY, posY;

            if (drawAbove)
            {
                posY = box.y - positionLabel.getHeight();
                idY  = posY - idLabel.getHeight();
            }
            else
            {
                idY  = box.y + box.h;
                posY = idY + idLabel.getHeight();
            }

            idLabel.render(idX, idY, m_renderer);
            positionLabel.render(posX, posY, m_renderer);
        }

    private:
        SDL_Window *m_window;
        SDL_Renderer *m_renderer;
        FontManager m_fontManager;

        TextAttributes m_playerLabelAttributes;
};
