#pragma once

#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../common/Logging.h"

class TextureManager
{
    public:
        ~TextureManager();

        bool loadTexture(const char* file, SDL_Renderer* renderer);
        SDL_Texture* getTexture(const char* file);
        void removeTexture(const char* file);

    private:
        std::map<const char*, SDL_Texture*> m_textures;
};
