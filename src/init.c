#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "init.h"
#include "audio.h"
#include "globals.h"
#include "texture.h"
#include "sampler.h"
#include "pipeline.h"
#include "gltf.h"
#include "text.h"

bool Init_RenderTargets()
{
	SDL_GetWindowSizeInPixels(window, &window_width, &window_height);

	SDL_WaitForGPUIdle(gpu_device);

    // virtual_screen_texture_height = window_height;
    virtual_screen_texture_width = virtual_screen_texture_height * window_width / window_height;
    
    if (virtual_screen_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, virtual_screen_texture);
    }
    virtual_screen_texture = SDL_CreateGPUTexture
    (
        gpu_device,
        &(SDL_GPUTextureCreateInfo)
        {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .width = virtual_screen_texture_width,
            .height = virtual_screen_texture_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER
        }
    );
    if (virtual_screen_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create virtual screen texture: %s", SDL_GetError());
        return false;
    }

	if (depth_texture)
	{
		SDL_ReleaseGPUTexture(gpu_device, depth_texture);
	}
	depth_texture = SDL_CreateGPUTexture
	(
		gpu_device,
		&(SDL_GPUTextureCreateInfo)
		{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.width = virtual_screen_texture_width,
			.height = virtual_screen_texture_height,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = msaa_level,
			.format = depth_texture_format, // Match pipeline
			.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
		}
	);
	if (depth_texture == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to recreate depth texture: %s", SDL_GetError());
		return false;
	}
    
    if (msaa_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, msaa_texture);
    }
    msaa_texture = SDL_CreateGPUTexture
    (
        gpu_device,
        &(SDL_GPUTextureCreateInfo)
        {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .width = virtual_screen_texture_width,
            .height = virtual_screen_texture_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = msaa_level, // MSAA
            .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
        }
    );
    if (msaa_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to recreate MSAA texture: %s", SDL_GetError());
        return false;
    }

	return true;
}

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
    
    if (!SDL_WindowSupportsGPUSwapchainComposition(gpu_device, window, swapchain_composition))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window does not support GPU swapchain composition! SDL Error: %s\n", SDL_GetError());
        swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Fallback to SDR
    }

    if (!SDL_WindowSupportsGPUPresentMode(gpu_device, window, swapchain_present_mode))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window does not support GPU present mode %d! SDL Error: %s\n", swapchain_present_mode, SDL_GetError());
        swapchain_present_mode = SDL_GPU_PRESENTMODE_VSYNC; // Fallback to VSync
    }

    if (swapchain_present_mode == SDL_GPU_PRESENTMODE_IMMEDIATE)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Immediate present mode may cause tearing! Consider using VSync or Mailbox instead.");
        // TODO how to abstract this for the user? settings generally just have a flag to disable/enable vsync
        manage_frame_rate_manually = true;
    }

    if (!SDL_SetGPUSwapchainParameters
        (
            gpu_device,
            window,
            swapchain_composition,
            swapchain_present_mode
        ))
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to set GPU swapchain parameters! SDL Error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!Pipeline_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize graphics pipelines");
        return SDL_APP_FAILURE;
    }
    
    SDL_LogTrace(SDL_LOG_CATEGORY_GPU, "All graphics pipelines initialized successfully.");

    if (!Sampler_Init())
    {
        return SDL_APP_FAILURE;
    }
    
    Init_RenderTargets();

    if (!Model_LoadAllModels())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to load models");
        return SDL_APP_FAILURE;
    }

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

    // AUDIO

    if (!Audio_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_AUDIO, "Failed to initialize audio!");
        return SDL_APP_FAILURE;
    }

    // TEXT

    if (!Text_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize text!");
        return SDL_APP_FAILURE;
    }

    // other initializations

    keyboard_state = (bool*)SDL_GetKeyboardState(NULL);
    
    performance_frequency = SDL_GetPerformanceFrequency();
    last_ticks = SDL_GetPerformanceCounter();

    return SDL_APP_CONTINUE; // Success
}