#ifndef BUTTON_H_
#define BUTTON_H_

#include <functional>
#include <string>
#include <SDL2/SDL.h>

#include "text.h"

class Button {
    public:
        Button() {}

        Button(int x, int y, int width, int height)
        : m_rect({x, y, width, height})
        , m_mainColor{60, 60, 60, 255}
        , m_hoverColor{90, 90, 90, 255}
        , m_shadowColor{20, 20, 20, 100}
        , m_text(nullptr)
        , m_isHover(false)
        , m_isPressed(false)
        , m_onClick(nullptr) {}

        void setText(std::string text, TextConfig textConfig, SDL_Renderer *renderer)
        {
            if (!m_text)
            {
                m_text = std::make_unique<Text>(textConfig);
            }
            m_text->setText(text, renderer);
        }

        void setMainColor(SDL_Color color)
        {
            m_mainColor = color;
        }

        void setHoverColor(SDL_Color color)
        {
            m_hoverColor = color;
        }

        void setShadowColor(SDL_Color color)
        {
            m_shadowColor = color;
        }

        void setOnClick(std::function<void()> callback)
        {
            m_onClick = std::move(callback);
        }

        void render(SDL_Renderer* renderer) {
            SDL_Rect renderRect = m_rect;

            if (!m_isPressed)
            {
                SDL_Rect shadowRect = {m_rect.x, m_rect.y + 4, m_rect.w, m_rect.h};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, m_shadowColor.r, m_shadowColor.g, m_shadowColor.b, 100);
                SDL_RenderFillRect(renderer, &shadowRect);
            }
            else
            {
                renderRect.y += 4;
            }

            if (m_isHover)
            {
                SDL_SetRenderDrawColor(renderer, m_hoverColor.r, m_hoverColor.g, m_hoverColor.b, 255);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, m_mainColor.r, m_mainColor.g, m_mainColor.b, 255);
            }

            SDL_RenderFillRect(renderer, &renderRect);

            if (m_text)
            {
                m_text->render(renderRect.x + (renderRect.w - m_text->getWidth()) / 2,
                               renderRect.y + (renderRect.h - m_text->getHeight()) / 2,
                               renderer);
            }
        }

        void handleEvent(const SDL_Event& e) {
            if (e.type == SDL_MOUSEMOTION)
            {
                SDL_Point mouse{e.motion.x, e.motion.y};
                bool wasHovered = m_isHover;

                m_isHover = SDL_PointInRect(&mouse, &m_rect);
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                SDL_Point mouse{e.button.x, e.button.y};
                if (SDL_PointInRect(&mouse, &m_rect))
                {
                    m_isPressed = true;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                SDL_Point mouse{e.button.x, e.button.y};
                if (m_isPressed && SDL_PointInRect(&mouse, &m_rect))
                {
                    if (m_onClick)
                    {
                        m_onClick();
                    }
                }
                m_isPressed = false;
            }
        }

        int getX()
        {
            return m_rect.x;
        }

        int getY()
        {
            return m_rect.y;
        }

    private:
        SDL_Rect m_rect;

        SDL_Color m_mainColor;
        SDL_Color m_hoverColor;
        SDL_Color m_shadowColor;

        std::unique_ptr<Text> m_text;

        bool m_isHover;
        bool m_isPressed;

        std::function<void()> m_onClick;
};

#endif