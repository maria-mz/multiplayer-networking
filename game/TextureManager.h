#ifndef TEXTURE_MANAGER_H_
#define TEXTURE_MANAGER_H_

#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../Logging.h"

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

#endif