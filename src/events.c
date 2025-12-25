#include "events.h"
#include "globals.h"
#include "audio.h"
#include "camera.h"


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        } break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Window size changed event");
            renderer_needs_to_be_reinitialized = true;
            return SDL_APP_CONTINUE;
        } break;
        default:
            switch(input_state)
            {
                case InputState_DEBUG:
                    if (!HandleEvent_InputState_DEBUG(event))
                    {
                        return SDL_APP_FAILURE;
                    } break;
                case InputState_FIRSTPERSONCONTROLLER:
                    if (!HandleEvent_InputState_FIRSTPERSONCONTROLLER(event))
                    {
                        return SDL_APP_FAILURE;
                    } break;
                default:
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unhandled input state %d", input_state);
                    break;
            }
    }
    return SDL_APP_CONTINUE;
}

bool HandleEvent_InputState_DEBUG(SDL_Event* event)
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
                mouse_xrel = (float)event->motion.xrel;
                mouse_yrel = (float)event->motion.yrel;
            }
        } break;
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_ESCAPE:
                {
                    if (is_mouse_captured) 
                    {
                        SDL_SetWindowRelativeMouseMode(window, false);
                        is_mouse_captured = false;
                    }
                } break;
                case SDL_SCANCODE_R:
                {
                    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Event: request to reload assets and reinitialize renderer");
                    renderer_needs_to_be_reinitialized = true;
                } break;
                case SDL_SCANCODE_TAB:
                {
                    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Tab Pressed", "Tab key was pressed!", window);
                    input_state = InputState_FIRSTPERSONCONTROLLER;
                    camera_active = &player.camera;
                } break;
                case SDL_SCANCODE_C: Camera_Log(); break;
                case SDL_SCANCODE_0: Bit_Toggle(settings_render, SETTINGS_RENDER_SHOW_DEBUG_TEXTURE);      break;
                case SDL_SCANCODE_1: Bit_Toggle(settings_render, SETTINGS_RENDER_LINEARIZE_DEBUG_TEXTURE); break;
                case SDL_SCANCODE_2: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_SSAO);             break;
                case SDL_SCANCODE_3: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_SHADOWS);          break;
                case SDL_SCANCODE_4:
                {
                    // TODO the way the renderer is reinitialized currently overrides this flag, so this doesn't actually do anything
                    // Bit_Toggle(settings_render, SETTINGS_RENDER_USE_LINEAR_FILTERING);
                    // renderer_needs_to_be_reinitialized = true;
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Linear Filtering must currently be overridden by changing the settings file and reloading the renderer");
                } break;
                case SDL_SCANCODE_5: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_FOG);   break;
                case SDL_SCANCODE_6: Bit_Toggle(settings_render, SETTINGS_RENDER_UPSCALE_SSAO); break;
                case SDL_SCANCODE_7: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_BLOOM); break;
                default: break;
            }
        } break;
        default: break;
    }
    return true;
}


bool HandleEvent_InputState_FIRSTPERSONCONTROLLER(SDL_Event* event)
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
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                mouse_clickedLeft = true;
            }
        } break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (is_mouse_captured)
            {
                mouse_xrel = (float)event->motion.xrel;
                mouse_yrel = (float)event->motion.yrel;
            }
        } break;
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_ESCAPE:
                {
                    if (is_mouse_captured) 
                    {
                        SDL_SetWindowRelativeMouseMode(window, false);
                        is_mouse_captured = false;
                    }
                } break;
                case SDL_SCANCODE_R:
                {
                    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Event: request to reload assets and reinitialize renderer");
                    renderer_needs_to_be_reinitialized = true;
                } break;
                case SDL_SCANCODE_TAB:
                {
                    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Tab Pressed", "Tab key was pressed!", window);
                    input_state = InputState_DEBUG;
                    camera_active = &camera_noClip;
                } break;
                case SDL_SCANCODE_C: Camera_Log(); break;
                case SDL_SCANCODE_0: Bit_Toggle(settings_render, SETTINGS_RENDER_SHOW_DEBUG_TEXTURE);      break;
                case SDL_SCANCODE_1: Bit_Toggle(settings_render, SETTINGS_RENDER_LINEARIZE_DEBUG_TEXTURE); break;
                case SDL_SCANCODE_2: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_SSAO);             break;
                case SDL_SCANCODE_3: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_SHADOWS);          break;
                case SDL_SCANCODE_4:
                {
                    // TODO the way the renderer is reinitialized currently overrides this flag, so this doesn't actually do anything
                    // Bit_Toggle(settings_render, SETTINGS_RENDER_USE_LINEAR_FILTERING);
                    // renderer_needs_to_be_reinitialized = true;
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Linear Filtering must currently be overridden by changing the settings file and reloading the renderer");
                } break;
                case SDL_SCANCODE_5: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_FOG);   break;
                case SDL_SCANCODE_6: Bit_Toggle(settings_render, SETTINGS_RENDER_UPSCALE_SSAO); break;
                case SDL_SCANCODE_7: Bit_Toggle(settings_render, SETTINGS_RENDER_ENABLE_BLOOM); break;
                case SDL_SCANCODE_SPACE:
                {
                    // Player_Print(&player);
                } break;
                default: break;
            }
        } break;
        default: break;
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
    // if (models_unanimated) 
    // {
    //     for (Uint32 i = 0; i < models_unanimated.len; i++)
    //     {
    //         Model_Free(models_unanimated + i);
    //     }
    //     SDL_free(models_unanimated);
    //     SDL_memset(&models_unanimated, 0, sizeof(Array_Model));
    // }
    if (sampler_albedo) SDL_ReleaseGPUSampler(gpu_device, sampler_albedo);
    if (window && gpu_device) SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    if (gpu_device) SDL_DestroyGPUDevice(gpu_device);
    if (window) SDL_DestroyWindow(window);
}