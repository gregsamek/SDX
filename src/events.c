#include "events.h"
#include "globals.h"
#include "audio.h"

typedef bool (*HandleEventFunction)(SDL_Event *event);

static HandleEventFunction HandleEventFunctions[InputState_COUNT] =
{
    [InputState_DEFAULT] = HandleEvent_InputState_DEFAULT,
};


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Window size changed event");
            window_resized = true;
            return SDL_APP_CONTINUE;
        }
        default:
        if (!HandleEventFunctions[input_state](event))
        {
            return SDL_APP_FAILURE;
        }
    }
    return SDL_APP_CONTINUE;
}

bool HandleEvent_InputState_DEFAULT(SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            if (event->button.button == SDL_BUTTON_LEFT && !is_mouse_captured)
            {
                SDL_SetWindowRelativeMouseMode(window, true);
                is_mouse_captured = true;
            }
        } break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (is_mouse_captured)
            {
                // TODO handle mouse inversion setting
                float xoffset = (float)-event->motion.xrel * mouse_sensitivity;
                float yoffset = (float)-event->motion.yrel * mouse_sensitivity;
            }
        } break;
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.scancode == SDL_SCANCODE_ESCAPE)
            {
                if (is_mouse_captured) 
                {
                    SDL_SetWindowRelativeMouseMode(window, false);
                    is_mouse_captured = false;
                }
            }
            else if (event->key.scancode == SDL_SCANCODE_R) // RELOAD MODELS
            {
                if (!Model_LoadAllModels())
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload models");
                    return false;
                }
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Models reloaded successfully.");
            }
            else if (event->key.scancode == SDL_SCANCODE_TAB)
            {
                if (!Audio_PlayTestSound())
                {
                    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to play test sound");
                    return false;
                }
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Tab Pressed", "Tab key was pressed!", window);
            }
        } break;
    }
    return true;
}

bool HandleWindowResize()
{
	int old_window_width = window_width;
	int old_window_height = window_height;
	SDL_GetWindowSizeInPixels(window, &window_width, &window_height);

	SDL_WaitForGPUIdle(gpu_device);

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
			.width = (Uint32)window_width,
			.height = (Uint32)window_height,
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
            .width = (Uint32)window_width,
            .height = (Uint32)window_height,
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

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (is_mouse_captured) SDL_SetWindowRelativeMouseMode(window, false); // Release mouse if captured
    
    TTF_DestroyText(text_renderable.ttf_text);
    TTF_DestroyGPUTextEngine(textEngine);
    TTF_CloseFont(font);
    TTF_Quit();
    
    SDL_WaitForGPUIdle(gpu_device); // Wait for GPU to finish all commands
    if (text_transfer_buffer) SDL_ReleaseGPUTransferBuffer(gpu_device, text_transfer_buffer);
    if (pipeline_unanimated) SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_unanimated);
    if (pipeline_bone_animated) SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_bone_animated);
    // if (pipeline_rigid_animated) SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_rigid_animated);
    // if (pipeline_instanced) SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_instanced);
    if (msaa_texture) SDL_ReleaseGPUTexture(gpu_device, msaa_texture);
    if (depth_texture) SDL_ReleaseGPUTexture(gpu_device, depth_texture);
    if (models_unanimated.arr) 
    {
        for (Uint32 i = 0; i < models_unanimated.len; i++)
        {
            Model_Free(models_unanimated.arr + i);
        }
        SDL_free(models_unanimated.arr);
        SDL_memset(&models_unanimated, 0, sizeof(Array_Model));
    }
    if (default_texture_sampler) SDL_ReleaseGPUSampler(gpu_device, default_texture_sampler);
    if (window && gpu_device) SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    if (gpu_device) SDL_DestroyGPUDevice(gpu_device);
    if (window) SDL_DestroyWindow(window);
}