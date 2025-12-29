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
                    Constants::FILE_FONT_MAIN.c_str(), 14
                );
                m_playerLabelAttributes.color = {0, 0, 0, 255};
                m_playerLabelAttributes.outlinePx = 0;

                m_hudTextAttributes.font = m_fontManager.getFont(
                    Constants::FILE_FONT_MAIN.c_str(), 14
                );
                m_hudTextAttributes.color = {0, 0, 0, 255};
                m_hudTextAttributes.outlinePx = 0;
            }

            return success;
        }

        void renderGame(GameSimulation& gameSimulation,
                        Constants::TransportType transportType)
        {
            assert(m_renderer != nullptr);

            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderClear(m_renderer);

            renderGrid();

            for (auto& [id, player] : gameSimulation.getPlayers())
            {
                renderPlayer(*player, id);
            }

            renderHUD(gameSimulation, transportType);

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
                !m_fontManager.loadFont(Constants::FILE_FONT_MAIN.c_str(), 14)
            )
            {
                success = false;
            }

            return success;
        }

        void renderPlayer(Player& player, PlayerID playerID)
        {
            assert(m_renderer != nullptr);

            std::string idText = std::format("{}", playerID);
            Text idLabel = Text(m_playerLabelAttributes, idText, m_renderer);

            SDL_Rect box = player.getBoundingBox();

            // Draw fill
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 255, 64);
            SDL_RenderFillRect(m_renderer, &box);

            // Draw border
            int borderAlpha  = player.isLocal ? 225 : 55;

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 150, borderAlpha);
            SDL_RenderDrawRect(m_renderer, &box);
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);

            // Draw label
            int centerX = box.x + box.w / 2;
            int topY = box.y - idLabel.getHeight();
            bool drawAbove = topY >= 0;

            int idX = centerX - idLabel.getWidth() / 2;
            int idY = drawAbove ? box.y - idLabel.getHeight() :  box.y + box.h;

            idLabel.render(idX, idY, m_renderer);
        }

        void renderGrid()
        {
            assert(m_renderer != nullptr);

            constexpr int GRID_SPACING = 16;
            constexpr int MAJOR_LINE_EVERY = 8;   // thicker line every N cells

            SDL_Color minorColor = { 0, 0, 0, 20 };
            SDL_Color majorColor = { 0, 0, 0, 40 };

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            // Draw vertical lines
            for (int x = 0; x <= Constants::WINDOW_WIDTH; x += GRID_SPACING)
            {
                bool isMajor = ((x / GRID_SPACING) % MAJOR_LINE_EVERY) == 0;
                SDL_Color c = isMajor ? majorColor : minorColor;

                SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
                SDL_RenderDrawLine(m_renderer, x, 0, x, Constants::WINDOW_HEIGHT);
            }

            // Draw horizontal lines
            for (int y = 0; y <= Constants::WINDOW_HEIGHT; y += GRID_SPACING)
            {
                bool isMajor = ((y / GRID_SPACING) % MAJOR_LINE_EVERY) == 0;
                SDL_Color c = isMajor ? majorColor : minorColor;

                SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
                SDL_RenderDrawLine(m_renderer, 0, y, Constants::WINDOW_WIDTH, y);
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        void renderHUD(GameSimulation& gameSimulation,
                       Constants::TransportType transportType)
        {
            assert(m_renderer != nullptr);

            const int padding = 8;
            const int startX = 10;
            const int startY = 10;
            const int lineSpacing = 2;

            std::vector<std::string> lines;

            lines.push_back(std::format("Transport: {}", transportTypeToString(transportType)));
            lines.push_back("");

            for (auto& [id, player] : gameSimulation.getPlayers())
            {
                lines.push_back(std::format(
                    "ID {} ({:.1f}, {:.1f})",
                    id,
                    player->m_position.x,
                    player->m_position.y
                ));
            }

            int cursorY = startY;
            const int textPaddingX = 6;
            const int textPaddingY = 2;

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            for (const auto& line : lines)
            {
                if (line.empty())
                {
                    cursorY += 6;
                    continue;
                }

                Text t(m_hudTextAttributes, line, m_renderer);

                SDL_Rect bgRect {
                    startX - textPaddingX,
                    cursorY - textPaddingY,
                    t.getWidth() + textPaddingX * 2,
                    t.getHeight() + textPaddingY * 2
                };

                // Draw background
                SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 190);
                SDL_RenderFillRect(m_renderer, &bgRect);

                // Draw outline
                SDL_SetRenderDrawColor(m_renderer, 200, 200, 200, 220);
                SDL_RenderDrawRect(m_renderer, &bgRect);

                // Draw text
                t.render(startX, cursorY, m_renderer);

                cursorY += t.getHeight() + lineSpacing;
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        std::string transportTypeToString(Constants::TransportType t)
        {
            switch (t)
            {
                case Constants::TransportType::TCP:  return "TCP";
                case Constants::TransportType::UDP:  return "UDP";
                default: return "Unknown";
            }
        }

    private:
        SDL_Window *m_window;
        SDL_Renderer *m_renderer;
        FontManager m_fontManager;

        TextAttributes m_playerLabelAttributes;
        TextAttributes m_hudTextAttributes;
};
