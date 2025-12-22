#ifndef TEXT_FIELD_H_
#define TEXT_FIELD_H_

#include <functional>
#include <string>
#include <SDL2/SDL.h>

#include "Text.h"

class TextField {
    public:
        TextField() {}

        TextField(int x, int y, int width, int height, TTF_Font *textFont) : m_rect({x, y, width, height})
        {
            TextConfig textConfig = {textFont, {255, 255, 255}};
            TextConfig textHintConfig = {textFont, {120, 120, 120}};

            m_textUI = std::make_unique<Text>(textConfig);
            m_hintTextUI = std::make_unique<Text>(textHintConfig);
        }

        void setTextHint(std::string text, SDL_Renderer *renderer)
        {
            m_hintTextUI->setText(text, renderer);
        }

        std::string getText()
        {
            return m_text;
        }

        void render(SDL_Renderer* renderer)
        {
            // update text if changed
            if (m_textUI && m_textUI->getText() != m_text)
            {
                m_textUI->setText(m_text, renderer);
            }

            // draw text field
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &m_rect);

            // set outline color
            if (m_isActive)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
            }

            // draw outline
            SDL_RenderDrawRect(renderer, &m_rect);

            // draw text
            if (m_hintTextUI && m_text.empty())
            {
                m_hintTextUI->render(m_rect.x + 10,
                                     m_rect.y + (m_rect.h - m_hintTextUI->getHeight()) / 2,
                                     renderer);
            }
            else if (m_textUI)
            {
                m_textUI->render(m_rect.x + 10,
                                 m_rect.y + (m_rect.h - m_textUI->getHeight()) / 2,
                                 renderer);
            }
        }

        void handleEvent(const SDL_Event& e)
        {
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int mx = e.button.x;
                int my = e.button.y;
                SDL_Point point{mx, my};
                m_isActive = SDL_PointInRect(&point, &m_rect);
            }
            else if (m_isActive && e.type == SDL_TEXTINPUT)
            {
                m_text += e.text.text;
            }
            else if (m_isActive && e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_BACKSPACE && !m_text.empty())
                {
                    m_text.pop_back();
                }
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

        std::unique_ptr<Text> m_textUI;
        std::string m_text;

        std::unique_ptr<Text> m_hintTextUI;

        bool m_isActive;
};

#endif