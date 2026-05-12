#pragma once

#include <cassert>
#include <format>
#include <string>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "common/logging.h"
#include "text.h"

#include "player.h"
#include "font_manager.h"
#include "game_simulation.h"


class RenderSystem
{
    public:
        struct Config
        {
            std::string windowTitle = "Pixelators";

            std::string mainFontFile = std::string(ASSETS_DIR) + "/Inconsolata.ttf";

            SDL_Color backgroundColor = {255, 255, 255, 255};
            SDL_Color textColor = {0, 0, 0, 255};

            SDL_Color localPlayerFillColor = {225, 238, 255, 255};
            SDL_Color remotePlayerFillColor = {246, 248, 252, 255};
            SDL_Color localPlayerBorderColor = {40, 105, 220, 255};
            SDL_Color remotePlayerBorderColor = {90, 95, 105, 255};

            int healthBarHeightPx = 5;
            int healthBarGapPx = 5;
            float lowHealthThreshold = 0.25f;
            SDL_Color healthBarTrackColor = {35, 35, 35, 155};
            SDL_Color healthBarBorderColor = {20, 20, 20, 185};
            SDL_Color healthBarNormalColor = {75, 175, 95, 235};
            SDL_Color healthBarLowColor = {210, 70, 65, 235};

            int aimIndicatorDistanceFromPlayerPx = 14;
            int aimIndicatorLengthPx = 10;
            int aimIndicatorWidthPx = 6;
            SDL_Color aimIndicatorColor = {40, 105, 220, 190};

            SDL_Color projectileFillColor = {105, 125, 145, 230};
            SDL_Color projectileBorderColor = {55, 70, 85, 255};

            int gridSpacingPx = 16;
            int gridMajorLineEvery = 8;
            SDL_Color gridMinorColor = {0, 0, 0, 20};
            SDL_Color gridMajorColor = {0, 0, 0, 40};

            int debugUIStartX = 10;
            int debugUIStartY = 10;
            int debugUILineSpacingPx = 2;
            int debugUIEmptyLineSpacingPx = 6;
            int debugUITextLeftRightPaddingPx = 6;
            int debugUITextTopDownPaddingPx = 2;
            SDL_Color debugUIBackgroundColor = {255, 255, 255, 190};
            SDL_Color debugUIBorderColor = {200, 200, 200, 220};
        };

    public:
        RenderSystem()
        : RenderSystem(Config{})
        {}

        RenderSystem(Config config)
        : m_config(config)
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
                    m_config.mainFontFile.c_str(), 16
                );
                m_playerLabelAttributes.color = m_config.textColor;
                m_playerLabelAttributes.outlinePx = 0;

                m_debugUITextAttributes.font = m_fontManager.getFont(
                    m_config.mainFontFile.c_str(), 16
                );
                m_debugUITextAttributes.color = m_config.textColor;
                m_debugUITextAttributes.outlinePx = 0;
            }

            return success;
        }

        void renderGame(GameSimulation& gameSimulation,
                        std::optional<float> ping,
                        double remoteUpdateVariability,
                        bool showDebugUI)
        {
            assert(m_renderer != nullptr);

            setRenderColor(m_config.backgroundColor);
            SDL_RenderClear(m_renderer);

            if (showDebugUI)
            {
                renderGrid();
            }

            renderPlayers(gameSimulation);

            for (const auto& [id, projectile] : gameSimulation.getProjectiles())
            {
                renderProjectile(projectile);
            }

            if (showDebugUI)
            {
                renderDebugUI(gameSimulation, ping, remoteUpdateVariability);
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
                m_window = SDL_CreateWindow(m_config.windowTitle.c_str(),
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        constants::WINDOW_WIDTH,
                                        constants::WINDOW_HEIGHT,
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
                setRenderColor(m_config.backgroundColor);

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
                !m_fontManager.loadFont(m_config.mainFontFile.c_str(), 8) ||
                !m_fontManager.loadFont(m_config.mainFontFile.c_str(), 12) ||
                !m_fontManager.loadFont(m_config.mainFontFile.c_str(), 16)
            )
            {
                success = false;
            }

            return success;
        }

        void setRenderColor(const SDL_Color& color)
        {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
        }

        void renderPlayers(GameSimulation& gameSimulation)
        {
            std::optional<PlayerID> localPlayerID = std::nullopt;

            for (auto& [id, player] : gameSimulation.getPlayers())
            {
                if (!player->isLocal)
                {
                    renderPlayer(*player, id);
                }
                else {
                    localPlayerID = id;
                }
            }

            // Render local player last so it is always easily visible
            if (localPlayerID)
            {
                auto& localPlayer = gameSimulation.getPlayer(*localPlayerID);
                renderPlayer(*localPlayer, *localPlayerID);
            }
        }

        void renderPlayer(Player& player, PlayerID playerID)
        {
            assert(m_renderer != nullptr);

            std::string idText = std::format("{}", playerID);
            Text idLabel = Text(m_playerLabelAttributes, idText, m_renderer);

            SDL_Rect box = player.getBoundingBox();

            // Draw fill
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            if (player.isLocal)
            {
                setRenderColor(m_config.localPlayerFillColor);
            }
            else
            {
                setRenderColor(m_config.remotePlayerFillColor);
            }
            SDL_RenderFillRect(m_renderer, &box);

            // Draw border
            if (player.isLocal)
            {
                setRenderColor(m_config.localPlayerBorderColor);
                SDL_RenderDrawRect(m_renderer, &box);
            }
            else
            {
                setRenderColor(m_config.remotePlayerBorderColor);
                SDL_RenderDrawRect(m_renderer, &box);
            }
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);

            renderPlayerHealthBar(player, box);

            int idX = box.x + (box.w - idLabel.getWidth()) / 2;
            int idY = box.y + (box.h - idLabel.getHeight()) / 2;
            idLabel.render(idX, idY, m_renderer);

            if (player.isLocal)
            {
                renderAimIndicator(player, box);
            }
        }

        void renderPlayerHealthBar(Player& player, const SDL_Rect& playerBox)
        {
            assert(m_renderer != nullptr);

            int health = player.getHealth();
            assert(health >= 0 && health <= Player::MAX_HEALTH);

            float healthRatio = static_cast<float>(health) / Player::MAX_HEALTH;

            SDL_Rect track {
                playerBox.x,
                playerBox.y - m_config.healthBarGapPx - m_config.healthBarHeightPx,
                playerBox.w,
                m_config.healthBarHeightPx
            };

            if (track.y < 0)
            {
                track.y = playerBox.y + playerBox.h + m_config.healthBarGapPx;
            }

            SDL_Rect fill = track;
            fill.w = static_cast<int>(track.w * healthRatio);

            SDL_Color fillColor = (healthRatio <= m_config.lowHealthThreshold)
                ? m_config.healthBarLowColor
                : m_config.healthBarNormalColor;

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            setRenderColor(m_config.healthBarTrackColor);
            SDL_RenderFillRect(m_renderer, &track);

            if (fill.w > 0)
            {
                setRenderColor(fillColor);
                SDL_RenderFillRect(m_renderer, &fill);
            }

            setRenderColor(m_config.healthBarBorderColor);
            SDL_RenderDrawRect(m_renderer, &track);

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        void renderAimIndicator(Player& player, const SDL_Rect& playerBox)
        {
            assert(m_renderer != nullptr);

            Vector2D<float> center{
                playerBox.x + playerBox.w / 2.0f,
                playerBox.y + playerBox.h / 2.0f
            };

            float aimDistance = playerBox.w / 2.0f + m_config.aimIndicatorDistanceFromPlayerPx;

            const Vector2D<float> forward = player.m_facingDirection;
            const Vector2D<float> perpendicular{-forward.y, forward.x};

            const Vector2D<float> tip = center + (forward * aimDistance);
            const Vector2D<float> base = tip - (forward * static_cast<float>(m_config.aimIndicatorLengthPx));
            const Vector2D<float> wingA = base + (perpendicular * static_cast<float>(m_config.aimIndicatorWidthPx));
            const Vector2D<float> wingB = base - (perpendicular * static_cast<float>(m_config.aimIndicatorWidthPx));

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            fillTriangle(tip, wingA, wingB, m_config.aimIndicatorColor);
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        void fillTriangle(Vector2D<float> a,
                          Vector2D<float> b,
                          Vector2D<float> c,
                          SDL_Color color)
        {
            SDL_Vertex vertices[3] = {};

            vertices[0].position = {a.x, a.y};
            vertices[0].color = color;

            vertices[1].position = {b.x, b.y};
            vertices[1].color = color;

            vertices[2].position = {c.x, c.y};
            vertices[2].color = color;

            SDL_RenderGeometry(m_renderer, nullptr, vertices, 3, nullptr, 0);
        }

        void renderProjectile(const Projectile& projectile)
        {
            assert(m_renderer != nullptr);

            SDL_Rect box = projectile.getBoundingBox();

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            setRenderColor(m_config.projectileFillColor);
            SDL_RenderFillRect(m_renderer, &box);
            setRenderColor(m_config.projectileBorderColor);
            SDL_RenderDrawRect(m_renderer, &box);

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        void renderGrid()
        {
            assert(m_renderer != nullptr);

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            // Draw vertical lines
            for (int x = 0; x <= constants::WINDOW_WIDTH; x += m_config.gridSpacingPx)
            {
                bool isMajor = ((x / m_config.gridSpacingPx) % m_config.gridMajorLineEvery) == 0;
                SDL_Color c = isMajor ? m_config.gridMajorColor : m_config.gridMinorColor;

                setRenderColor(c);
                SDL_RenderDrawLine(m_renderer, x, 0, x, constants::WINDOW_HEIGHT);
            }

            // Draw horizontal lines
            for (int y = 0; y <= constants::WINDOW_HEIGHT; y += m_config.gridSpacingPx)
            {
                bool isMajor = ((y / m_config.gridSpacingPx) % m_config.gridMajorLineEvery) == 0;
                SDL_Color c = isMajor ? m_config.gridMajorColor : m_config.gridMinorColor;

                setRenderColor(c);
                SDL_RenderDrawLine(m_renderer, 0, y, constants::WINDOW_WIDTH, y);
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

        void renderDebugUI(GameSimulation& gameSimulation,
                           std::optional<float> ping,
                           double remoteUpdateVariability)
        {
            assert(m_renderer != nullptr);

            std::vector<std::string> lines;

            std::string pingText = ping.has_value() ? std::format("{:.0f}", *ping) : "--";

            lines.push_back(std::format("Ping: {} ms", pingText));
            lines.push_back("");
            lines.push_back(std::format("Remote update variability: {:.2f}", remoteUpdateVariability));
            lines.push_back("");

            for (auto& [id, player] : gameSimulation.getPlayers())
            {
                lines.push_back(std::format(
                    "ID {} HP {} ({:.1f}, {:.1f})",
                    id,
                    player->getHealth(),
                    player->m_position.x,
                    player->m_position.y
                ));
            }

            int cursorY = m_config.debugUIStartY;

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

            for (const auto& line : lines)
            {
                if (line.empty())
                {
                    cursorY += m_config.debugUIEmptyLineSpacingPx;
                    continue;
                }

                Text t(m_debugUITextAttributes, line, m_renderer);

                SDL_Rect bgRect {
                    m_config.debugUIStartX - m_config.debugUITextLeftRightPaddingPx,
                    cursorY - m_config.debugUITextTopDownPaddingPx,
                    t.getWidth() + m_config.debugUITextLeftRightPaddingPx * 2,
                    t.getHeight() + m_config.debugUITextTopDownPaddingPx * 2
                };

                // Draw background
                setRenderColor(m_config.debugUIBackgroundColor);
                SDL_RenderFillRect(m_renderer, &bgRect);

                // Draw outline
                setRenderColor(m_config.debugUIBorderColor);
                SDL_RenderDrawRect(m_renderer, &bgRect);

                // Draw text
                t.render(m_config.debugUIStartX, cursorY, m_renderer);

                cursorY += t.getHeight() + m_config.debugUILineSpacingPx;
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }

    private:
        SDL_Window *m_window;
        SDL_Renderer *m_renderer;
        FontManager m_fontManager;
        Config m_config;

        TextAttributes m_playerLabelAttributes;
        TextAttributes m_debugUITextAttributes;
};
