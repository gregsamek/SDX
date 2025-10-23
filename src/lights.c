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
        
        int active_lights_count = 3;

        Light_Spot* lights_mapped = (Light_Spot*)mapped;
        // light 1
        {
            vec3 light_position_world = {0.0f, 5.0f, 5.0f};
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
            
            lights_mapped[0] = (Light_Spot)
            {
                .position = { light_position_viewspace[0], light_position_viewspace[1], light_position_viewspace[2] },
                .attenuation_constant_linear = 0.0f,
                .color = { 1.0f, 1.0f, 1.0f },
                .attenuation_constant_quadratic = 0.0f,
                .direction = { light_direction_viewspace[0], light_direction_viewspace[1], light_direction_viewspace[2] },
                .cutoff_inner = SDL_cosf(glm_rad(180.0f)),
                .cutoff_outer = SDL_cosf(glm_rad(180.0f))
            };
        }
        // light 2
        {
            vec3 light_position_world = {0.0f, 5.0f, -5.0f};
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
            
            lights_mapped[1] = (Light_Spot)
            {
                .position = { light_position_viewspace[0], light_position_viewspace[1], light_position_viewspace[2] },
                .attenuation_constant_linear = 0.0f,
                .color = { 1.0f, 1.0f, 1.0f },
                .attenuation_constant_quadratic = 0.0f,
                .direction = { light_direction_viewspace[0], light_direction_viewspace[1], light_direction_viewspace[2] },
                .cutoff_inner = SDL_cosf(glm_rad(180.0f)),
                .cutoff_outer = SDL_cosf(glm_rad(180.0f))
            };
        }
        // light 3
        {
            vec3 light_position_world = {5.0f, 0.0f, 0.0f};
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
            
            lights_mapped[2] = (Light_Spot)
            {
                .position = { light_position_viewspace[0], light_position_viewspace[1], light_position_viewspace[2] },
                .attenuation_constant_linear = 0.0f,
                .color = { 1.0f, 1.0f, 1.0f },
                .attenuation_constant_quadratic = 0.0f,
                .direction = { light_direction_viewspace[0], light_direction_viewspace[1], light_direction_viewspace[2] },
                .cutoff_inner = SDL_cosf(glm_rad(180.0f)),
                .cutoff_outer = SDL_cosf(glm_rad(180.0f))
            };
        }
        SDL_GPUTransferBufferLocation source = 
        {
            .transfer_buffer = lights_transfer_buffer,
            .offset = 0
        };
        
        SDL_GPUBufferRegion destination = 
        {
            .buffer = lights_storage_buffer,
            .offset = 0,
            .size = active_lights_count * sizeof(Light_Spot)
        };
            
        SDL_UploadToGPUBuffer(copyPass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, lights_transfer_buffer);
        SDL_EndGPUCopyPass(copyPass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

void Lights_UpdateShadowMatrices_Directional(vec3 light_dir_world)
{
    glm_vec3_normalize(light_dir_world);

    mat4 inverse_camera_view;
    glm_mat4_inv(camera.view_matrix, inverse_camera_view);

    vec3 cam_pos = { inverse_camera_view[3][0], inverse_camera_view[3][1], inverse_camera_view[3][2] };
    vec3 cam_fwd = { -inverse_camera_view[2][0], -inverse_camera_view[2][1], -inverse_camera_view[2][2] };

    // Focus the shadow box around a point in front of the camera
    vec3 focus;
    glm_vec3_scale(cam_fwd, 25.0f, focus); // 25m ahead
    glm_vec3_add(cam_pos, focus, focus);

    // Place the light a bit "back" along its direction
    vec3 light_pos;
    vec3 back;
    glm_vec3_scale(light_dir_world, -50.0f, back);
    glm_vec3_add(focus, back, light_pos);

    vec3 up = { 0.0f, 1.0f, 0.0f };
    glm_lookat(light_pos, focus, up, light_view_matrix);

    // Ortho projection covering a box around the focus point
    glm_ortho(-SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
              -SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
              SHADOW_NEAR, SHADOW_FAR, light_proj_matrix);

    // Cache VP
    glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
}