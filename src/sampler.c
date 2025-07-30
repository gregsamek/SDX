#include <SDL3/SDL.h>

#include "sampler.h"
#include "globals.h"

bool SamplerInit()
{
    default_texture_sampler = SDL_CreateGPUSampler
    (
        gpu_device, 
        &(SDL_GPUSamplerCreateInfo)
        {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .min_lod = 0.0f,
            .max_lod = 16.0f, // arbitrary limit (i.e. use every mip level)
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .enable_compare = false
        }
    );
    if (default_texture_sampler == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create sampler: %s", SDL_GetError());
        return false;
    }
    return true;
}