#include "TextureManager.h"


TextureManager::~TextureManager()
{
    for (auto &pair : m_textures)
    {
        SDL_DestroyTexture(pair.second);
        pair.second = nullptr;
    }
}

bool TextureManager::loadTexture(const char *file, SDL_Renderer* renderer)
{
    removeTexture(file);

    bool success = true;

    SDL_Texture* newTexture = NULL;

    SDL_Surface* loadedSurface = IMG_Load(file);
    if (loadedSurface == NULL)
    {
        success = false;
        LOG_ERROR("Couldn't load image. File: %s, Error: %s", file, IMG_GetError());
    }
    else
    {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == NULL)
        {
            success = false;

            LOG_ERROR(
                "Couldn't create texture from image. File: %s, Error: %s", file, IMG_GetError()
            );
        }

        SDL_FreeSurface(loadedSurface);
    }

    if (newTexture)
    {
        m_textures[file] = newTexture;
    }

    return success;
}

SDL_Texture* TextureManager::getTexture(const char *file)
{
    if (m_textures.find(file) != m_textures.end())
    {
        return m_textures[file];
    }

    return nullptr;
}

void TextureManager::removeTexture(const char *file)
{
    SDL_Texture *texture = getTexture(file);
    if (texture)
    {
        SDL_DestroyTexture(texture);
        m_textures.erase(file);
    }
}
