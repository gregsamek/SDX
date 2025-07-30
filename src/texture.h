#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

SDL_Surface* LoadImage(const char* imageFilename);
Uint8 GetTextureIndex(const char* filename);

#endif // TEXTURE_H