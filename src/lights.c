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

        vec3 light_position_world = {10.0f, 0.0f, 0.0f};
        vec3 light_target_world   = {0.0f, 0.0f, 0.0f};

        vec3 light_direction_world;
        glm_vec3_sub(light_target_world, light_position_world, light_direction_world);
        
        vec4 light_position_world_4 = { light_position_world[0], light_position_world[1], light_position_world[2], 1.0f };
        vec4 light_position_viewspace_4;
        glm_mat4_mulv(camera.view_matrix, light_position_world_4, light_position_viewspace_4);
        vec3 light_position_viewspace = { light_position_viewspace_4[0], light_position_viewspace_4[1], light_position_viewspace_4[2] };
        
        vec4 light_direction_world_4 = { light_direction_world[0], light_direction_world[1], light_direction_world[2], 0.0f };
        vec4 light_direction_viewspace_4;
        glm_mat4_mulv(camera.view_matrix, light_direction_world_4, light_direction_viewspace_4);
        vec3 light_direction_viewspace = { light_direction_viewspace_4[0], light_direction_viewspace_4[1], light_direction_viewspace_4[2] };
        glm_vec3_normalize(light_direction_viewspace);
        
        // Spotlight
        lights_mapped[0].position[0] = light_position_viewspace[0];
        lights_mapped[0].position[1] = light_position_viewspace[1];
        lights_mapped[0].position[2] = light_position_viewspace[2];
        lights_mapped[0].attenuation_constant_linear = 0.09f;
        lights_mapped[0].color[0] = 0.0f;
        lights_mapped[0].color[1] = 1.0f;
        lights_mapped[0].color[2] = 0.0f;
        lights_mapped[0].attenuation_constant_quadratic = 0.032f;
        lights_mapped[0].direction[0] = light_direction_viewspace[0];
        lights_mapped[0].direction[1] = light_direction_viewspace[1];
        lights_mapped[0].direction[2] = light_direction_viewspace[2];
        lights_mapped[0].cutoff_inner = SDL_cosf(glm_rad(0.0f)); // inner cone angle
        lights_mapped[0].cutoff_outer = SDL_cosf(glm_rad(30.0f)); // outer cone angle

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