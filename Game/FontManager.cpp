#include "FontManager.h"

FontManager::~FontManager()
{
    for (auto &pair : m_fonts)
    {
        TTF_CloseFont(pair.second);
        pair.second = nullptr;
    }
}

bool FontManager::loadFont(const char *file, int fontSizePx)
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

TTF_Font* FontManager::getFont(const char *file, int fontSizePx)
{
    if (m_fonts.find({file, fontSizePx}) != m_fonts.end())
    {
        return m_fonts[{file, fontSizePx}];
    }

    return nullptr;
}

void FontManager::removeFont(const char *file, int fontSizePx)
{
    TTF_Font *font = getFont(file, fontSizePx);
    if (font)
    {
        TTF_CloseFont(font);
        m_fonts.erase({file, fontSizePx});
    }
}
