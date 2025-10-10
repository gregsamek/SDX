#include "lights.h"
#include "globals.h"

bool Lights_StorageBuffer_UpdateAndUpload()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    {
        void* mapped = SDL_MapGPUTransferBuffer(gpu_device, lights_transfer_buffer, true);
        if (!mapped) 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
            return false;
        }
        
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(command_buffer);
        
        // Fill in light data here TODO this is just test data

        int active_lights_count = 1;

        Light_Spotlight* lights_mapped = (Light_Spotlight*)mapped;

        // Spotlight
        lights_mapped[0].position[0] = 0.0f;
        lights_mapped[0].position[1] = 5.0f;
        lights_mapped[0].position[2] = 0.0f;
        lights_mapped[0].attenuation_constant_linear = 0.09f;
        lights_mapped[0].color[0] = 1.0f;
        lights_mapped[0].color[1] = 1.0f;
        lights_mapped[0].color[2] = 1.0f;
        lights_mapped[0].attenuation_constant_quadratic = 0.032f;
        lights_mapped[0].direction[0] = 0.0f;
        lights_mapped[0].direction[1] = -1.0f;
        lights_mapped[0].direction[2] = 0.0f;
        lights_mapped[0].cutoff_inner = SDL_cosf(glm_rad(12.5f)); // inner cone angle
        lights_mapped[0].cutoff_outer = SDL_cosf(glm_rad(17.5f)); // outer cone angle

        SDL_GPUTransferBufferLocation source = 
        {
            .transfer_buffer = lights_transfer_buffer,
            .offset = 0
        };
        
        SDL_GPUBufferRegion destination = 
        {
            .buffer = lights_storage_buffer,
            .offset = 0,
            .size = active_lights_count * sizeof(Light_Spotlight)
        };
            
        SDL_UploadToGPUBuffer(copyPass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, lights_transfer_buffer);
        SDL_EndGPUCopyPass(copyPass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}