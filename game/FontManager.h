#pragma once

#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../common/Logging.h"

class FontManager
{
    public:
        ~FontManager()
        {
            for (auto &pair : m_fonts)
            {
                TTF_CloseFont(pair.second);
                pair.second = nullptr;
            }
        }

        bool loadFont(const char* file, int fontSizePx)
        {
            removeFont(file, fontSizePx);

            bool success = true;

            TTF_Font* font = TTF_OpenFont(file, fontSizePx);
            if (!font)
            {
                LOG_ERROR("Couldn't load font. File: %s, Error: %s", file, TTF_GetError());
                success = false;
            }
            else
            {
                m_fonts[{file, fontSizePx}] = font;
            }

            return success;
        }

        TTF_Font* getFont(const char* file, int fontSizePx)
        {
            if (m_fonts.find({file, fontSizePx}) != m_fonts.end())
            {
                return m_fonts[{file, fontSizePx}];
            }

            return nullptr;
        }

        void removeFont(const char* file, int fontSizePx)
        {
            TTF_Font *font = getFont(file, fontSizePx);
            if (font)
            {
                TTF_CloseFont(font);
                m_fonts.erase({file, fontSizePx});
            }
        }

    private:
        std::map<std::pair<const char*, int>, TTF_Font*> m_fonts;
};
