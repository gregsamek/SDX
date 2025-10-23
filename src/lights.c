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

        Light_Spotlight* lights_mapped = (Light_Spotlight*)mapped;
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
            
            lights_mapped[0] = (Light_Spotlight)
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
            
            lights_mapped[1] = (Light_Spotlight)
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
            
            lights_mapped[2] = (Light_Spotlight)
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
            .size = active_lights_count * sizeof(Light_Spotlight)
        };
            
        SDL_UploadToGPUBuffer(copyPass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, lights_transfer_buffer);
        SDL_EndGPUCopyPass(copyPass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return true;
}

void ComputeDirectionalLightShadowMatrices(vec3 light_dir_world)
{
    // Your world-space light direction (normalized)
    glm_vec3_normalize(light_dir_world);

    // Derive camera world position and forward from view matrix
    mat4 inv_view;
    glm_mat4_inv(camera.view_matrix, inv_view);

    vec3 cam_pos = { inv_view[3][0], inv_view[3][1], inv_view[3][2] };
    vec3 cam_fwd = { -inv_view[2][0], -inv_view[2][1], -inv_view[2][2] };

    // Focus the shadow box around a point in front of the camera
    vec3 focus;
    glm_vec3_scale(cam_fwd, 25.0f, focus); // 25m ahead
    glm_vec3_add(cam_pos, focus, focus);

    // Place the light a bit "back" along its direction
    vec3 light_pos;
    vec3 back;
    glm_vec3_scale(light_dir_world, -50.0f, back);
    glm_vec3_add(focus, back, light_pos);

    // Build light view
    vec3 up = { 0.0f, 1.0f, 0.0f };
    glm_lookat(light_pos, focus, up, light_view_matrix);

    // Ortho projection covering a box around the focus point
    glm_ortho(-SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
              -SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
              SHADOW_NEAR, SHADOW_FAR, light_proj_matrix);

    // Cache VP
    glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
}

// void ComputeDirectionalLightShadowMatrices(vec3 light_dir_world)
// {
//     glm_vec3_normalize(light_dir_world);

//     vec3 target = { 0.0f, 0.0f, 0.0f };   // choose a suitable world point
//     vec3 light_pos;
//     glm_vec3_scale_as(light_dir_world, -50.0f, light_pos); // move “upstream” of the light
//     glm_vec3_add(target, light_pos, light_pos);

//     vec3 up = { 0.0f, 1.0f, 0.0f };

//     glm_lookat(light_pos, target, up, light_view_matrix);
//     glm_ortho(-SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
//               -SHADOW_ORTHO_HALF, SHADOW_ORTHO_HALF,
//               SHADOW_NEAR, SHADOW_FAR, light_proj_matrix);

//     glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
// }

// void ComputeDirectionalLightShadowMatrices(vec3 light_dir_world)
// {
//     glm_vec3_normalize(light_dir_world);

//     // 1) Build a light view orientation (no camera position baked in)
//     vec3 up = {0, 1, 0};
//     if (fabsf(glm_vec3_dot(up, light_dir_world)) > 0.99f)
//         up = (vec3){1, 0, 0};

//     // We'll place the light later; first get the light rotation/basis
//     mat4 light_view_rot;
//     {
//         // Look from origin in direction of -light_dir to get rotation only
//         vec3 eye = {0,0,0};
//         vec3 center = {-light_dir_world[0], -light_dir_world[1], -light_dir_world[2]};
//         glm_lookat(eye, center, up, light_view_rot);
//     }

//     // 2) Get the 8 world-space frustum corners for [near, farShadow]
//     // Choose a far shadow distance suitable for your scene
//     float nearDist = camera.near_plane;
//     float farDist  = /* e.g. */ 50.0f;
//     vec3 frustumWS[8];
//     ExtractCameraFrustumCornersWS(camera.projection_matrix, camera.view_matrix,
//                                   nearDist, farDist, frustumWS);

//     // 3) Transform them to light space and compute bounds
//     vec3 minLS = { FLT_MAX,  FLT_MAX,  FLT_MAX};
//     vec3 maxLS = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
//     for (int i = 0; i < 8; i++)
//     {
//         vec4 p = { frustumWS[i][0], frustumWS[i][1], frustumWS[i][2], 1.0f };
//         vec4 pLS;
//         glm_mat4_mulv(light_view_rot, p, pLS); // rotation-only view
//         minLS[0] = fminf(minLS[0], pLS[0]);
//         minLS[1] = fminf(minLS[1], pLS[1]);
//         minLS[2] = fminf(minLS[2], pLS[2]);
//         maxLS[0] = fmaxf(maxLS[0], pLS[0]);
//         maxLS[1] = fmaxf(maxLS[1], pLS[1]);
//         maxLS[2] = fmaxf(maxLS[2], pLS[2]);
//     }

//     // Pad a bit to account for receiver/PCF
//     float pad = 2.0f; // world units
//     minLS[0] -= pad; minLS[1] -= pad;
//     maxLS[0] += pad; maxLS[1] += pad;

//     // 4) Stabilize by snapping XY bounds to shadow texels
//     float w = maxLS[0] - minLS[0];
//     float h = maxLS[1] - minLS[1];
//     float worldPerTexelX = w / (float)SHADOW_MAP_SIZE;
//     float worldPerTexelY = h / (float)SHADOW_MAP_SIZE;

//     minLS[0] = floorf(minLS[0] / worldPerTexelX) * worldPerTexelX;
//     minLS[1] = floorf(minLS[1] / worldPerTexelY) * worldPerTexelY;
//     maxLS[0] = floorf(maxLS[0] / worldPerTexelX) * worldPerTexelX;
//     maxLS[1] = floorf(maxLS[1] / worldPerTexelY) * worldPerTexelY;

//     // 5) Place the light so that its view center is at the snapped center
//     vec3 centerLS = { 0.5f * (minLS[0] + maxLS[0]),
//                       0.5f * (minLS[1] + maxLS[1]),
//                       0.5f * (minLS[2] + maxLS[2]) };

//     // Transform center back to world to get light position/target
//     mat4 invLightRot;
//     glm_mat4_inv(light_view_rot, invLightRot);
//     vec4 centerWS4;
//     glm_mat4_mulv(invLightRot, (vec4){centerLS[0], centerLS[1], centerLS[2], 1.0f}, centerWS4);
//     vec3 centerWS = {centerWS4[0], centerWS4[1], centerWS4[2]};

//     vec3 lightPosWS;
//     vec3 lightFwdWS = {-light_dir_world[0], -light_dir_world[1], -light_dir_world[2]};
//     // Put the light "behind" the box along +light_dir to cover depth range
//     // Offset so near/far will enclose minLS.z..maxLS.z after we set the proj
//     // With an ortho camera, you can place eye at centerWS minus some z; simplest:
//     glm_vec3_copy(centerWS, lightPosWS);

//     // Final light view
//     glm_lookat(lightPosWS, (vec3){centerWS[0] + lightFwdWS[0],
//                                   centerWS[1] + lightFwdWS[1],
//                                   centerWS[2] + lightFwdWS[2]},
//                up, light_view_matrix);

//     // 6) Build ortho from snapped min/max
//     // Depth: ensure near/far cover [minLS.z, maxLS.z] in the light space produced by light_view_matrix.
//     // If you use OpenGL-style clip depth [-1,1], keep your shader's z remap. If 0..1, don't remap in shader.
//     float nearLS = -maxLS[2]; // depending on your light view forward
//     float farLS  = -minLS[2];
//     glm_ortho(minLS[0], maxLS[0], minLS[1], maxLS[1], nearLS, farLS, light_proj_matrix);

//     glm_mat4_mul(light_proj_matrix, light_view_matrix, light_viewproj_matrix);
// }