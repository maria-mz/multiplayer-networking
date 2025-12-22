#ifndef TEXT_H_
#define TEXT_H_

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct TextConfig
{
    TTF_Font *font;
    SDL_Color color;
    int outlinePx = 0;
    SDL_Color outlineColor = {0, 0, 0};
};

class Text
{
    public:
        Text() {}

        Text(TextConfig config)
        : m_config(config)
        , m_text("")
        , m_texture(nullptr)
        , m_surfaceWidth(0)
        , m_surfaceHeight(0) {}

        ~Text()
        {
            destroyText();
        }

        std::string getText()
        {
            return m_text;
        }

        int getWidth()
        {
            return m_surfaceWidth;
        }

        int getHeight()
        {
            return m_surfaceHeight;
        }

        void setText(std::string text, SDL_Renderer* renderer)
        {
            if (m_text == text)
            {
                return;
            }

            destroyText();

            m_text = text;

            if (m_text.empty()) {
                m_texture = nullptr;
                m_surfaceWidth = 0;
                m_surfaceHeight = 0;
                return;
            }

            if (m_config.outlinePx > 0)
            {
                int originalOutline = TTF_GetFontOutline(m_config.font);

                TTF_SetFontOutline(m_config.font, m_config.outlinePx);
                SDL_Surface* outlineSurface = TTF_RenderText_Blended(m_config.font, m_text.c_str(), m_config.outlineColor);

                TTF_SetFontOutline(m_config.font, originalOutline); // reset outline on font

                SDL_Surface* mainSurface = TTF_RenderText_Blended(m_config.font, m_text.c_str(), m_config.color);

                // Blit main text onto outlined background
                SDL_BlitSurface(mainSurface, NULL, outlineSurface, NULL);

                m_texture = SDL_CreateTextureFromSurface(renderer, outlineSurface);
                m_surfaceWidth = outlineSurface->w;
                m_surfaceHeight = outlineSurface->h;

                SDL_FreeSurface(outlineSurface);
                SDL_FreeSurface(mainSurface);
            }
            else
            {
                SDL_Surface* mainSurface = TTF_RenderText_Blended(m_config.font, m_text.c_str(), m_config.color);

                m_texture = SDL_CreateTextureFromSurface(renderer, mainSurface);
                m_surfaceWidth = mainSurface->w;
                m_surfaceHeight = mainSurface->h;

                SDL_FreeSurface(mainSurface);
            }
        }

        void render(int x, int y, SDL_Renderer* renderer)
        {
            if (m_text.empty())
            {
                return;
            }

            SDL_Rect dstRect = {x, y, m_surfaceWidth, m_surfaceHeight};
            SDL_RenderCopy(renderer, m_texture, NULL, &dstRect);
        }

    private:
        void destroyText()
        {
            SDL_DestroyTexture(m_texture);
        }

        TextConfig m_config;
        std::string m_text;

        SDL_Texture *m_texture;
        int m_surfaceWidth;
        int m_surfaceHeight;
};

#endif