#ifndef SPRITE_H
#define SPRITE_H

#include <SDL3/SDL.h>

#include "array.h"
#include "helper.h"

Struct (Sprite)
{
    SDL_GPUTexture* texture;
    float aspect_ratio;
    float height;
    Sint16 x;
    Sint16 y;
    Uint8 _padding[4];
};

bool Sprite_LoadSprites(void);

#endif // SPRITE_H