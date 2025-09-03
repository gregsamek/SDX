#include "texture.h"
#include "globals.h"

SDL_Surface* LoadImage(const char* imageFilename)
{
	char fullPath[MAXIMUM_URI_LENGTH];
	SDL_Surface *result;
	SDL_PixelFormat format = SDL_PIXELFORMAT_RGBA32;

	SDL_snprintf(fullPath, sizeof(fullPath), "%stextures/%s", base_path, imageFilename);

	result = IMG_Load(fullPath);
	if (result == NULL)
	{
		SDL_Log("Failed to load image: %s", SDL_GetError());
		return NULL;
	}

	if (result->format != format)
	{
		SDL_Surface *converted_surface = SDL_ConvertSurface(result, format);
		SDL_DestroySurface(result);
		if (!converted_surface)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not convert surface to any RGBA32 format: %s", SDL_GetError());
			return NULL;
		}
		result = converted_surface;
	}

	return result;
}

Uint8 GetTextureIndex(const char* filename)
{
    // This function is a placeholder for texture index retrieval logic.
    static Uint16 texture_index = 0;
    return texture_index++;
}