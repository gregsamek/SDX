#include <SDL3/SDL.h>

#include "sampler.h"
#include "globals.h"

bool Sampler_Init()
{
    SDL_GPUSamplerCreateInfo sampler_create_info =
    {
        // Note: these three settings may be overridden below if linear filtering is desired
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        
        .min_lod = 0.0f,
        .max_lod = 16.0f, // arbitrary limit (i.e. use every mip level)
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .enable_anisotropy = true,
        .max_anisotropy = 16.0f,
        .enable_compare = false
    };

    if (settings_render & SETTINGS_RENDER_USE_LINEAR_FILTERING)
    {
        sampler_create_info.min_filter = SDL_GPU_FILTER_LINEAR;
        sampler_create_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    }
    if (sampler_albedo)
    {
        SDL_ReleaseGPUSampler(gpu_device, sampler_albedo);
        sampler_albedo = NULL;
    }
    sampler_albedo = SDL_CreateGPUSampler
    (
        gpu_device, 
        &sampler_create_info
    );
    if (sampler_albedo == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create sampler: %s", SDL_GetError());
        return false;
    }

    if (sampler_nearest_nomips)
    {
        SDL_ReleaseGPUSampler(gpu_device, sampler_nearest_nomips);
        sampler_nearest_nomips = NULL;
    }
    sampler_nearest_nomips = SDL_CreateGPUSampler
    (
        gpu_device,
        &(SDL_GPUSamplerCreateInfo)
        {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 1.0f,
            .compare_op = SDL_GPU_COMPAREOP_INVALID, // manual compare in shader
            .min_lod = 0.0f,
            .max_lod = 0.0f,
            .enable_anisotropy = false,
            .enable_compare = false,
        }
    );
    if (!sampler_nearest_nomips)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create shadow sampler: %s", SDL_GetError());
        return false;
    }

    if (sampler_linear_nomips)
    {
        SDL_ReleaseGPUSampler(gpu_device, sampler_linear_nomips);
        sampler_linear_nomips = NULL;
    }
    sampler_linear_nomips = SDL_CreateGPUSampler
    (
        gpu_device,
        &(SDL_GPUSamplerCreateInfo)
        {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 1.0f,
            .compare_op = SDL_GPU_COMPAREOP_INVALID, // manual compare in shader
            .min_lod = 0.0f,
            .max_lod = 0.0f,
            .enable_anisotropy = false,
            .enable_compare = false,
        }
    );
    if (!sampler_linear_nomips)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create shadow sampler: %s", SDL_GetError());
        return false;
    }

    return true;
}