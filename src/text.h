#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "helper.h"

// TODO what would be good defaults for these? should be enough for the longest string, right?
// maybe just make them configurable
#define MAX_TEXT_VERTEX_COUNT 4000
#define MAX_TEXT_INDEX_COUNT  6000

Struct (Text_Vertex)
{
    SDL_FPoint xy;
    SDL_FPoint uv;
};

Struct (Text_Renderable)
{
    TTF_Text *ttf_text;
    TTF_GPUAtlasDrawSequence* draw_sequence;
    SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
    Uint32 vertex_count;
	Uint32 index_count;
};

bool Text_Init();
bool Text_UpdateAndUpload(const char* new_text);

#endif // TEXT_H