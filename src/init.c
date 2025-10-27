#include <SDL3_ttf/SDL_ttf.h>

#include "init.h"
#include "audio.h"
#include "globals.h"
#include "texture.h"
#include "gltf.h"
#include "text.h"
#include "sprite.h"
#include "render.h"
#include "lights.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);
    // SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_TRACE);

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    SDL_SetAppMetadata("SDL Game", "0", "com.example.sdlgame");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    base_path = SDL_GetBasePath();

    gpu_device = SDL_CreateGPUDevice
    (
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true, // Enable debug mode for more detailed logs
        NULL // Let SDL pick the graphics driver
    );

    if (gpu_device == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    window = SDL_CreateWindow("SDL GPU glTF Viewer", 1352, 815, window_flags);
    if (window == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_SetWindowBordered(window, false);

    SDL_Surface* icon = LoadImage("test_texture.png");
    SDL_SetWindowIcon(window, icon);
    SDL_DestroySurface(icon);

    SDL_GetWindowSizeInPixels(window, &window_width, &window_height);

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // depth texture for main render pass
    if (SDL_GPUTextureSupportsFormat(gpu_device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    {
        depth_texture_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    else if (SDL_GPUTextureSupportsFormat(gpu_device, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    {
        depth_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    }
    else 
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "WARNING: Depth texture format fell back to D16_UNORM", NULL);
        depth_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    }

    // we need to test this separately because the above format may not support sampling usage as well
    // maybe I should just use the one that supports sampling for both?
    if (SDL_GPUTextureSupportsFormat(gpu_device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER))
    {
        depth_sample_texture_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    else if (SDL_GPUTextureSupportsFormat(gpu_device, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER))
    {
        depth_sample_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    }
    else 
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "WARNING: Shadow map texture format fell back to D16_UNORM", NULL);
        depth_sample_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    }

    if (!Render_LoadRenderSettings())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load render settings");
        return SDL_APP_FAILURE;
    }
    
    if (!Render_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize renderer!");
        return SDL_APP_FAILURE;
    }

    Array_Init(sprites, 1);
    if (!sprites)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize sprites array");
        return SDL_APP_FAILURE;
    }

    if (!Sprite_LoadSprites())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprites");
        return SDL_APP_FAILURE;
    }

    Array_Init(models_unanimated, 1);
    if (!models_unanimated)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize models array");
        return SDL_APP_FAILURE;
    }

    Array_Init(models_bone_animated, 1);
    if (!models_bone_animated)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize models_bone_animated array");
        return SDL_APP_FAILURE;
    }

    if (!Model_Load_AllScenes())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to load models");
        return SDL_APP_FAILURE;
    }

    Array_Init(lights_spot, MAX_TOTAL_LIGHTS);
    if (!lights_spot)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize spot lights array");
        return SDL_APP_FAILURE;
    }
    if (!Lights_LoadLights())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to load lights");
        return SDL_APP_FAILURE;
    }

    // STORAGE BUFFERS ////////////////////////////////////////////////////////

    // TODO size joint buffers appropriately based on the number of joints in the loaded models
    joint_matrix_storage_buffer = SDL_CreateGPUBuffer
    (
        gpu_device, 
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = MAX_TOTAL_JOINTS_TO_RENDER * sizeof(mat4)
        }
    );
    if (joint_matrix_storage_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bone matrix storage buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    joint_matrix_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = MAX_TOTAL_JOINTS_TO_RENDER * sizeof(mat4)
        }
    );
    if (joint_matrix_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bone matrix transfer buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    lights_storage_buffer = SDL_CreateGPUBuffer
    (
        gpu_device, 
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = MAX_TOTAL_LIGHTS * sizeof(Light_Spot)
        }
    );
    if (lights_storage_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bone matrix storage buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    lights_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = MAX_TOTAL_JOINTS_TO_RENDER * sizeof(mat4)
        }
    );
    if (lights_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bone matrix transfer buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // AUDIO //////////////////////////////////////////////////////////////////

    if (!Audio_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_AUDIO, "Failed to initialize audio!");
        return SDL_APP_FAILURE;
    }

    // TEXT ///////////////////////////////////////////////////////////////////

    if (!Text_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize text!");
        return SDL_APP_FAILURE;
    }

    // OTHER //////////////////////////////////////////////////////////////////

    keyboard_state = (bool*)SDL_GetKeyboardState(NULL);
    
    performance_frequency = SDL_GetPerformanceFrequency();
    last_ticks = SDL_GetPerformanceCounter();

    return SDL_APP_CONTINUE;
}