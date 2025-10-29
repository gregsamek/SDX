#include "lights.h"
#include "globals.h"

bool Lights_Update()
{
    // Spot ///////////////////////////////////////////////////////////////////
    
    if (lights_storage_buffer) Lights_StorageBuffer_UpdateAndUpload();
    
    // current shadow casting only supports one shadow casting light
    bool any_shadow_casters = false;
    
    foreach(light_spot, lights_spot)
    {
        if (light_spot.shadow_caster)
        {
            Lights_UpdateShadowMatrices_Spot(&light_spot);
            any_shadow_casters = true;
        }
    }

    // Directional ////////////////////////////////////////////////////////////

    vec3 light_direction_world = {1.0f, -1.0f, 1.0f};
    vec4 light_direction_world_4 = { light_direction_world[0], light_direction_world[1], light_direction_world[2], 0.0f };
    vec4 light_direction_viewspace_4;
    glm_mat4_mulv(camera.view_matrix, light_direction_world_4, light_direction_viewspace_4);
    vec3 light_direction_viewspace = { light_direction_viewspace_4[0], light_direction_viewspace_4[1], light_direction_viewspace_4[2] };
    glm_vec3_normalize(light_direction_viewspace);

    light_directional = (Light_Directional)
    {
        .direction = {light_direction_viewspace[0], light_direction_viewspace[1], light_direction_viewspace[2]},
        .strength = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .shadow_caster = true
    };

    if (light_directional.shadow_caster)
    {
        SDL_assert(!any_shadow_casters); // only one shadow caster supported currently
        Lights_UpdateShadowMatrices_Directional(light_direction_world);
    }
    
    // Hemisphere /////////////////////////////////////////////////////////////

    vec4 world_up_4 = { 0.0f, 1.0f, 0.0f, 0.0f };
    vec4 view_up_4;
    glm_mat4_mulv(camera.view_matrix, world_up_4, view_up_4);
    vec3 view_up = { view_up_4[0], view_up_4[1], view_up_4[2] };
    glm_vec3_normalize(view_up);

    light_hemisphere = (Light_Hemisphere)
    {
        .up_viewspace = {view_up[0], view_up[1], view_up[2]},
        .color_sky = {1.0f, 1.0f, 1.0f},
        .color_ground = {1.0f, 1.0f, 1.0f},
    };

    return true;
}

bool Lights_LoadLights()
{
    Light_Spot light_spot = 
    {
        .position = { 8.0f, 0.0f, 8.0f },
        .attenuation_constant_linear = 0.0f,
        .color = { 0.0f, 0.0f, 0.0f },
        .attenuation_constant_quadratic = 0.0f,
        .target = {0, 0, 0},
        .cutoff_inner = SDL_cosf(glm_rad(0.0f)),
        .cutoff_outer = SDL_cosf(glm_rad(10.0f)),
        .shadow_caster = false,
    };
    Array_Append(lights_spot, light_spot);
    return true;
}

// TODO if there are no lights, don't upload buffer
// also need to make the light count available to the shader
// (or just handle lights totally differently)
bool Lights_StorageBuffer_UpdateAndUpload()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    {
        Light_Spot* lights_mapped = SDL_MapGPUTransferBuffer(gpu_device, lights_transfer_buffer, true);
        if (!lights_mapped) 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
            return false;
        }
        
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        if (!copy_pass)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_BeginGPUCopyPass failed: %s", SDL_GetError());
            SDL_UnmapGPUTransferBuffer(gpu_device, lights_transfer_buffer);
            return false;
        }
        
        int active_lights_count = Array_Len(lights_spot);
        SDL_assert(active_lights_count <= MAX_TOTAL_LIGHTS);

        for (size_t i = 0; i < active_lights_count; i++)
        {
            Light_Spot light = lights_spot[i];

            vec3 light_direction_world;
            glm_vec3_sub(light.target, light.position, light_direction_world);

            vec4 light_position_world_4 = { light.position[0], light.position[1], light.position[2], 1.0f };
            vec4 light_position_viewspace_4;
            glm_mat4_mulv(camera.view_matrix, light_position_world_4, light_position_viewspace_4);
            vec3 light_position_viewspace = { light_position_viewspace_4[0], light_position_viewspace_4[1], light_position_viewspace_4[2] };
            
            vec4 light_direction_world_4 = { light_direction_world[0], light_direction_world[1], light_direction_world[2], 0.0f };
            vec4 light_direction_viewspace_4;
            glm_mat4_mulv(camera.view_matrix, light_direction_world_4, light_direction_viewspace_4);
            vec3 light_direction_viewspace = { light_direction_viewspace_4[0], light_direction_viewspace_4[1], light_direction_viewspace_4[2] };
            glm_vec3_normalize(light_direction_viewspace);
            
            lights_mapped[i] = light;
            glm_vec3_copy(light_position_viewspace, lights_mapped[i].position);
            glm_vec3_copy(light_direction_viewspace, lights_mapped[i].direction);
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
            
        SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, lights_transfer_buffer);
        SDL_EndGPUCopyPass(copy_pass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

// TODO I need to figure out a proper way to size these shadow maps
void Lights_UpdateShadowMatrices_Directional(vec3 light_dir_world)
{
    // glm_vec3_normalize(light_dir_world);

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
    glm_ortho(-SHADOW_ORTHO_HALF_WIDTH, SHADOW_ORTHO_HALF_WIDTH,
              -SHADOW_ORTHO_HALF_WIDTH, SHADOW_ORTHO_HALF_WIDTH,
              SHADOW_NEAR, SHADOW_FAR, light_proj_matrix);

    // Cache VP
    glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
    SHADOW_FAR = 150.0f;
}

void Lights_UpdateShadowMatrices_Spot(Light_Spot* light)
{
    vec3 up = { 0.0f, 1.0f, 0.0f };
    glm_lookat(light->position, light->target, up, light_view_matrix);

    // 'cutoff_outer' is stored as cos(half_angle)
    float fov = SDL_acosf(light->cutoff_outer) * 2.0f; // full cone angle in radians

    float aspect_ratio = 1.0f;
    glm_perspective(fov, aspect_ratio, SHADOW_NEAR, SHADOW_FAR, light_proj_matrix);

    // Cache VP
    glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
    SHADOW_FAR = 15.0f;
}