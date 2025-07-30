#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// TODO what would be good defaults for these?
// maybe just make them configurable
#define MAX_TEXT_VERTEX_COUNT 4000
#define MAX_TEXT_INDEX_COUNT  6000

typedef struct Text_Vertex
{
    SDL_FPoint xy;
    SDL_FPoint uv;
} Text_Vertex;

typedef struct
{
    TTF_Text *ttf_text;
    TTF_GPUAtlasDrawSequence* draw_sequence;
    SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
    Uint32 vertex_count;
	Uint32 index_count;
} Text_Renderable;

typedef struct Array_Text_Renderable
{
    Text_Renderable* arr;
    Uint32 len;
    Uint32 cap;
} Array_Text_Renderable;

bool Text_Init();
bool Text_UpdateAndUpload(const char* new_text);

#endif // TEXT_H