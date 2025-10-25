#include "render.h"
#include "globals.h"
#include "text.h"
#include "sampler.h"
#include "pipeline.h"
#include "lights.h"

////////// INITIALIZATION /////////////

bool Render_LoadRenderSettings()
{
    char path[MAXIMUM_URI_LENGTH];
    SDL_snprintf(path, sizeof(path), "%s%s", base_path, "settings.txt");
    size_t settings_txt_size = 0;
    char* settings_txt = (char*)SDL_LoadFile(path, &settings_txt_size);
    if (!settings_txt)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite list: %s", SDL_GetError());
        return false;
    }

    char* saveptr = NULL;
    char* line = SDL_strtok_r(settings_txt, "\r\n", &saveptr);

    while (line != NULL)
    {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') 
            line++;
        
        // Skip empty lines
        if (*line != '\0')
        {
            char* setting_name = line;

            char* setting_value; 
            SDL_strtok_r(line, " ", &setting_value);

            if (SDL_strcmp(setting_name, "virtual_screen_texture_height") == 0)
            {
                virtual_screen_texture_height = (Uint32)SDL_strtoul(setting_value, NULL, 10);
            }
            else if (SDL_strcmp(setting_name, "msaa_level") == 0)
            {
                int msaa = SDL_strtol(setting_value, NULL, 10);
                switch (msaa)
                {
                    case 1: msaa_level = SDL_GPU_SAMPLECOUNT_1; break;
                    case 2: msaa_level = SDL_GPU_SAMPLECOUNT_2; break;
                    case 4: msaa_level = SDL_GPU_SAMPLECOUNT_4; break;
                    default:
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid MSAA level %d in settings.txt, using 1x MSAA", msaa);
                        msaa_level = SDL_GPU_SAMPLECOUNT_1;
                        break;
                }
            }
            else if (SDL_strcmp(setting_name, "use_linear_filtering") == 0)
            {
                use_linear_filtering = SDL_strtol(setting_value, NULL, 10);
            }
            else if (SDL_strcmp(setting_name, "n_mipmap_levels") == 0)
            {
                n_mipmap_levels = (Uint32)SDL_strtoul(setting_value, NULL, 10);
            }
            else if (SDL_strcmp(setting_name, "vsync") == 0)
            {
                int vsync = SDL_strtol(setting_value, NULL, 10);
                if (vsync == 0)
                {
                    swapchain_present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
                }
                else
                {
                    swapchain_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
                }
            }
            else if (SDL_strcmp(setting_name, "maximum_frame_rate") == 0)
            {
                minimum_frame_time = 1.0 / (double)SDL_strtol(setting_value, NULL, 10);
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unknown setting name in settings.txt: %s", setting_name);
            }
        }
        
        line = SDL_strtok_r(NULL, "\r\n", &saveptr);
    }

    SDL_free(settings_txt);
    return true;
}

bool Render_InitRenderTargets()
{
	SDL_GetWindowSizeInPixels(window, &window_width, &window_height);

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
            .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
		SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create depth texture: %s", SDL_GetError());
		return false;
	}

    if (shadow_map_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, shadow_map_texture);
    }
    shadow_map_texture = SDL_CreateGPUTexture
    (
        gpu_device,
        &(SDL_GPUTextureCreateInfo)
        {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = shadow_map_texture_format,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = SHADOW_MAP_SIZE,
            .height = SHADOW_MAP_SIZE,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        }
    );
    if (shadow_map_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create shadow map texture: %s", SDL_GetError());
        return false;
    }
    shadow_ubo = (ShadowUBO){.texel_size = {1.0f / (float)SHADOW_MAP_SIZE, 1.0f / (float)SHADOW_MAP_SIZE}, .bias = SHADOW_BIAS, .pcf_radius = SHADOW_PCF_RADIUS};
    
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
            .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
        }
    );
    if (msaa_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create MSAA texture: %s", SDL_GetError());
        return false;
    }

	return true;
}

bool Render_Init()
{
    SDL_WaitForGPUIdle(gpu_device);

    // TODO: try for SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, if not supported, fall back to SDL_GPU_SWAPCHAINCOMPOSITION_SDR
    // adjust final swapchain shader to match the respective format
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
        // TODO check for MAILBOX support and make that option available
        // if MAILBOX is supported, use that as the default instead of IMMEDIATE
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
        return false;
    }

    if (!Pipeline_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize graphics pipelines");
        return false;
    }
    
    SDL_LogTrace(SDL_LOG_CATEGORY_GPU, "All graphics pipelines initialized successfully.");

    if (!Sampler_Init())
    {
        return false;
    }

    if (!Render_InitRenderTargets())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize render targets");
        return false;
    }

    return true;
}

////////// FRAME RENDERING /////////////

static void Render_Unanimated_Shadow(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!Array_Len(models_unanimated)) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_shadow_depth);

    for (size_t i = 0; i < Array_Len(models_unanimated); i++)
    {
        // TODO implement model matrix per model
        mat4 model_matrix;
        glm_mat4_identity(model_matrix);
        
        mat4 light_mvp_matrix;
        glm_mat4_mul(light_viewproj_matrix, model_matrix, light_mvp_matrix);

        SDL_PushGPUVertexUniformData
        (
            command_buffer, 
            0, // uniform buffer slot
            &light_mvp_matrix, 
            sizeof(mat4)
        );
        
        SDL_BindGPUVertexBuffers
        (
            render_pass, 
            0, // vertex buffer slot
            (SDL_GPUBufferBinding[])
            {
                { 
                    .buffer = models_unanimated[i].mesh.vertex_buffer, 
                    .offset = 0 
                },
            }, 
            1 // vertex buffer count
        );            
        
        SDL_BindGPUIndexBuffer
        (
            render_pass, 
            &(SDL_GPUBufferBinding)
            { 
                .buffer = models_unanimated[i].mesh.index_buffer, 
                .offset = 0 
            }, 
            SDL_GPU_INDEXELEMENTSIZE_16BIT
        );

        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            (Uint32)models_unanimated[i].mesh.index_count, // num_indices
            1,  // num_instances
            0,  // first_index
            0,  // vertex_offset
            0   // first_instance
        );
    }
}

static void Render_Unanimated(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!Array_Len(models_unanimated)) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_unanimated);

    for (size_t i = 0; i < Array_Len(models_unanimated); i++)
    {
        // TODO implement model matrix per model
        mat4 model_matrix;
        glm_mat4_identity(model_matrix);

        mat4 mv_matrix;
        glm_mat4_mul(camera.view_matrix, model_matrix, mv_matrix);
        
        mat4 mvp_matrix;
        glm_mat4_mul(camera.view_projection_matrix, model_matrix, mvp_matrix);

        // TODO we already calculated this in shadow pass; cache it
        mat4 light_mvp_model;
        glm_mat4_mul(light_viewproj_matrix, model_matrix, light_mvp_model);
        
        TransformsUBO transforms = {0};
        glm_mat4_copy(mvp_matrix, transforms.mvp);
        glm_mat4_copy(mv_matrix, transforms.mv);
        glm_mat4_copy(light_mvp_model, transforms.mvp_light);

#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
        // normal matrix = inverse-transpose of the upper-left 3x3 of mv
        mat3 mv3, normal3;
        glm_mat4_pick3(mv_matrix, mv3);     // take upper-left 3x3
        glm_mat3_inv(mv3, normal3);
        glm_mat3_transpose(normal3);
        glm_mat4_identity(transforms.normal);
        glm_mat4_ins3(normal3, transforms.normal);
#endif

        SDL_PushGPUVertexUniformData
        (
            command_buffer, 
            0, // uniform buffer slot
            &transforms, 
            sizeof(transforms)
        );
        
        SDL_BindGPUVertexBuffers
        (
            render_pass, 
            0, // vertex buffer slot
            (SDL_GPUBufferBinding[])
            {
                { 
                    .buffer = models_unanimated[i].mesh.vertex_buffer, 
                    .offset = 0 
                },
            }, 
            1 // vertex buffer count
        );            
        
        SDL_BindGPUIndexBuffer
        (
            render_pass, 
            &(SDL_GPUBufferBinding)
            { 
                .buffer = models_unanimated[i].mesh.index_buffer, 
                .offset = 0 
            }, 
            SDL_GPU_INDEXELEMENTSIZE_16BIT
        );

        SDL_GPUTexture* texture_diffuse = models_unanimated[i].mesh.material.texture_diffuse;    
        SDL_GPUTexture* texture_metallic_roughness = models_unanimated[i].mesh.material.texture_metallic_roughness;
        SDL_GPUTexture* texture_normal = models_unanimated[i].mesh.material.texture_normal;
        
        SDL_BindGPUFragmentSamplers
        (
            render_pass, 
            0, // first slot
            (SDL_GPUTextureSamplerBinding[])
            {
                { .texture = texture_diffuse,  .sampler = default_texture_sampler },
                { .texture = texture_metallic_roughness, .sampler = default_texture_sampler },
                { .texture = texture_normal, .sampler = default_texture_sampler },
                { .texture = shadow_map_texture, .sampler = shadow_sampler },
            },
            4 // num_bindings
        );

        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            (Uint32)models_unanimated[i].mesh.index_count, // num_indices
            1,  // num_instances
            0,  // first_index
            0,  // vertex_offset
            0   // first_instance
        );
    }
}

static void Render_BoneAnimated(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!Array_Len(models_bone_animated)) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_bone_animated);
    SDL_BindGPUVertexStorageBuffers
    (
        render_pass,
        0, // storage buffer slot
        &joint_matrix_storage_buffer,
        1 // storage buffer count
    );

    for (size_t i = 0; i < Array_Len(models_bone_animated); i++)
    {
        // TODO implement model matrix per model
        mat4 model_matrix;
        glm_mat4_identity(model_matrix);

        mat4 mv_matrix;
        glm_mat4_mul(camera.view_matrix, model_matrix, mv_matrix);
        
        mat4 mvp_matrix;
        glm_mat4_mul(camera.view_projection_matrix, model_matrix, mvp_matrix);
        
        TransformsUBO transforms = {0};
        glm_mat4_copy(mvp_matrix, transforms.mvp);
        glm_mat4_copy(mv_matrix, transforms.mv);

#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
        // normal matrix = inverse-transpose of the upper-left 3x3 of mv
        mat3 mv3, normal3;
        glm_mat4_pick3(mv_matrix, mv3);     // take upper-left 3x3
        glm_mat3_inv(mv3, normal3);
        glm_mat3_transpose(normal3);
        glm_mat4_identity(transforms.normal);
        glm_mat4_ins3(normal3, transforms.normal);
#endif

        SDL_PushGPUVertexUniformData
        (
            command_buffer, 
            0, 
            &transforms, 
            sizeof(transforms)
        );

        SDL_PushGPUVertexUniformData
        (
            command_buffer, 
            1, 
            &models_bone_animated[i].animation_rig.storage_buffer_offset_bytes, 
            sizeof(Uint32)
        );

        SDL_BindGPUVertexBuffers
        (
            render_pass, 
            0, // vertex buffer slot
            (SDL_GPUBufferBinding[])
            {
                { 
                    .buffer = models_bone_animated[i].model.mesh.vertex_buffer, 
                    .offset = 0 
                },
            }, 
            1 // vertex buffer count
        );            
        
        SDL_BindGPUIndexBuffer
        (
            render_pass, 
            &(SDL_GPUBufferBinding)
            { 
                .buffer = models_bone_animated[i].model.mesh.index_buffer, 
                .offset = 0 
            }, 
            SDL_GPU_INDEXELEMENTSIZE_16BIT
        );
        
        SDL_BindGPUFragmentSamplers
        (
            render_pass, 
            0, // fragment sampler slot
            &(SDL_GPUTextureSamplerBinding)
            { 
                .texture = models_bone_animated[i].model.mesh.material.texture_diffuse, 
                .sampler = default_texture_sampler 
            }, 
            1 // num_bindings
        );

        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            (Uint32)models_bone_animated[i].model.mesh.index_count, // num_indices
            1,  // num_instances
            0,  // first_index
            0,  // vertex_offset
            0   // first_instance
        );
    }
}

static bool Render_Text(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_text);
    
    char test_text[256];
    // snprintf(test_text, sizeof(test_text), "Camera Position: (%.2f, %.2f, %.2f)\nVertices: %d, Indices: %d", camera.position[0], camera.position[1], camera.position[2], text_renderable.vertex_count, text_renderable.index_count);
    // snprintf(test_text, sizeof(test_text), "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789\n!@#$%%^&*()_+[]{}|;':\",.<>?/~`");
    snprintf(test_text, sizeof(test_text), "%.0f", average_frame_rate);
    
    // TODO updating text should probably happen separately at the beginning of the frame 
    Text_UpdateAndUpload(test_text);
    
    int text_width, text_height;
    if (!TTF_GetTextSize(text_renderable.ttf_text, &text_width, &text_height))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get text size");
        return false;
    }

    // SDL_Log("Text Width: %d, Text Height: %d", text_width, text_height);

    // TODO this relative target should be specified in the text renderable
    float text_target_width = 0.0625f;
    float text_scale_correction = text_target_width * (float)virtual_screen_texture_width / (float)text_width;
    
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    // vec3 scale_vec = { text_scale_correction, text_scale_correction, 1.0f };
    // glm_scale(model_matrix, scale_vec);
    
    mat4 projection_matrix_ortho;
    glm_ortho(0.0f, (float)virtual_screen_texture_width, -(float)virtual_screen_texture_height, 0.0f, 0.0f, 1.0f, projection_matrix_ortho);
    // glm_ortho(0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, projection_matrix_ortho);
    
    mat4 mvp_matrix;
    glm_mat4_mul(projection_matrix_ortho, model_matrix, mvp_matrix);
    
    SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp_matrix, sizeof(mvp_matrix));

    SDL_BindGPUVertexBuffers
    (
        render_pass,
        0, // vertex buffer slot
        (SDL_GPUBufferBinding[])
        {
            { 
                .buffer = text_renderable.vertex_buffer, 
                .offset = 0 
            },
        },
        1 // vertex buffer count
    );

    SDL_BindGPUIndexBuffer
    (
        render_pass,
        &(SDL_GPUBufferBinding)
        { 
            .buffer = text_renderable.index_buffer, 
            .offset = 0 
        },
        SDL_GPU_INDEXELEMENTSIZE_32BIT
    );

    int index_offset = 0, vertex_offset = 0;
    for (TTF_GPUAtlasDrawSequence* sequence = text_renderable.draw_sequence; sequence != NULL; sequence = sequence->next)
    {
        SDL_BindGPUFragmentSamplers
        (
            render_pass,
            0, // fragment sampler slot
            &(SDL_GPUTextureSamplerBinding)
            { 
                .texture = sequence->atlas_texture, 
                .sampler = default_texture_sampler 
            },
            1 // num_bindings
        );
    
        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            sequence->num_indices, // num_indices
            1, // num_instances
            index_offset, // first_index
            vertex_offset, // vertex_offset
            0  // first_instance
        );

        index_offset += sequence->num_indices;
        vertex_offset += sequence->num_vertices;
    }

    return true;
}

static void Render_Sprite(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!Array_Len(sprites)) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_sprite);

    for (size_t i = 0; i < Array_Len(sprites); i++)
    {
        float sprite_height = sprites[i].height;
        float sprite_width = sprite_height * sprites[i].aspect_ratio * virtual_screen_texture_height / (float)virtual_screen_texture_width;
        float sprite_depth = 0.01f; // TODO depth should be a property of the sprite
        float vertex_pos[4][4] = 
        {
            {0.0f, 0.0f, sprite_depth, 1.0f}, // top left
            {sprite_width, 0.0f, sprite_depth, 1.0f}, // top right
            {0.0f, sprite_height, sprite_depth, 1.0f}, // bottom left
            {sprite_width, sprite_height, sprite_depth, 1.0f}  // bottom right
        };

        SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_pos, sizeof(vertex_pos));
        
        SDL_BindGPUFragmentSamplers
        (
            render_pass, 
            0, // fragment sampler slot
            &(SDL_GPUTextureSamplerBinding)
            { 
                .texture = sprites[i].texture, 
                .sampler = default_texture_sampler 
            }, 
            1 // num_bindings
        );

        SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
    }
}

bool Render()
{
    // TODO break this up into separate states e.g. assets need reloaded, piplines need reloaded, etc.
    if (renderer_needs_to_be_reinitialized)
    {
        if (!Render_LoadRenderSettings())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load render settings");
            return false;
        }
        foreach(model, models_unanimated)
        {
            Model_Free(&model);
        }
        Array_Len(models_unanimated) = 0;
        foreach(model_bone_animated, models_bone_animated)
        {
            Model_BoneAnimated_Free(&model_bone_animated);
        }
        Array_Len(models_bone_animated) = 0;
        if (!Model_Load_AllScenes())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload models");
            return false;
        }
        foreach(sprite, sprites)
        {
            SDL_ReleaseGPUTexture(gpu_device, sprite.texture);
        }
        Array_Len(sprites) = 0;
        if (!Sprite_LoadSprites())
        {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload sprites");
            return false;
        }
        if (!Render_Init())
        {
            return false;
        }
        Array_Len(lights_spot) = 0;
        if (!Lights_LoadLights())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload lights");
            return false;
        }
        renderer_needs_to_be_reinitialized = false;
    }

    // can these be compined into one copy pass? (also text update)
    if (Array_Len(models_bone_animated)) Model_JointMat_UpdateAndUpload();
    if (lights_storage_buffer) Lights_StorageBuffer_UpdateAndUpload();

    SDL_GPUCommandBuffer* command_buffer_draw = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (command_buffer_draw == NULL)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }

    // Update Directional Light

    vec3 light_direction_world = {-1.0f, -0.0f, 0.0f};
    vec4 light_direction_world_4 = { light_direction_world[0], light_direction_world[1], light_direction_world[2], 0.0f };
    vec4 light_direction_viewspace_4;
    glm_mat4_mulv(camera.view_matrix, light_direction_world_4, light_direction_viewspace_4);
    vec3 light_direction_viewspace = { light_direction_viewspace_4[0], light_direction_viewspace_4[1], light_direction_viewspace_4[2] };
    glm_vec3_normalize(light_direction_viewspace);

    Light_Directional light_directional = 
    {
        .direction = {light_direction_viewspace[0], light_direction_viewspace[1], light_direction_viewspace[2]},
        .strength = 0.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .shadow_caster = false
    };
    if (light_directional.shadow_caster)
        Lights_UpdateShadowMatrices_Directional(light_direction_world);
    
    foreach(light_spot, lights_spot)
    {
        if (light_spot.shadow_caster)
            Lights_UpdateShadowMatrices_Spot(&light_spot);
    }

    // Shadow Pass

    SDL_GPUDepthStencilTargetInfo shadow_target = 
    {
        .texture = shadow_map_texture,
        .clear_depth = 1.0f,
        .clear_stencil = 0,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true
    };

    SDL_GPURenderPass* shadow_pass = SDL_BeginGPURenderPass
    (
        command_buffer_draw,
        NULL, 
        0,
        &shadow_target
    );
    if (!shadow_pass)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin shadow pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }
    
    SDL_SetGPUViewport(shadow_pass, &(SDL_GPUViewport)
    {
        .x = 0, .y = 0,
        .w = (float)SHADOW_MAP_SIZE, .h = (float)SHADOW_MAP_SIZE,
        .min_depth = 0.0f, .max_depth = 1.0f
    });

    Render_Unanimated_Shadow(shadow_pass, command_buffer_draw);

    // TODO add bone animated shadow rendering (need a different vert shader)

    SDL_EndGPURenderPass(shadow_pass);

    // Main Pass

    SDL_GPUColorTargetInfo virtual_target_info = 
    {
        .texture = virtual_screen_texture,
        .clear_color = (SDL_FColor){ 0.5f, 0.5f, 0.5f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = true
    };

    if (msaa_level > SDL_GPU_SAMPLECOUNT_1)
    {
        virtual_target_info.texture = msaa_texture;
        virtual_target_info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
        virtual_target_info.resolve_texture = virtual_screen_texture;
        virtual_target_info.cycle_resolve_texture = true;
    }

    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = 
    {
        .texture = depth_texture,
        .clear_depth = 1.0f,
        .clear_stencil = 0,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE, // Don't need to store depth after frame
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true
    };

    SDL_GPURenderPass* virtual_render_pass = SDL_BeginGPURenderPass
    (
        command_buffer_draw,
        &virtual_target_info,
        1,
        &depth_stencil_target_info
    );

    if (!virtual_render_pass)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin virtual render pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    SDL_SetGPUViewport(virtual_render_pass, &(SDL_GPUViewport)
    { 
        .x = 0, 
        .y = 0,
        .w = (int)virtual_screen_texture_width, 
        .h = (int)virtual_screen_texture_height,
        .min_depth = 0.0f, 
        .max_depth = 1.0f
    });

    SDL_BindGPUFragmentStorageBuffers
    (
        virtual_render_pass,
        0, // storage buffer slot
        &lights_storage_buffer,
        1 // storage buffer count
    );

    SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &light_directional, sizeof(light_directional));

    {
        vec4 world_up_4 = { 0.0f, 1.0f, 0.0f, 0.0f };
        vec4 view_up_4;
        glm_mat4_mulv(camera.view_matrix, world_up_4, view_up_4);
        vec3 view_up = { view_up_4[0], view_up_4[1], view_up_4[2] };
        glm_vec3_normalize(view_up);

        Light_Hemisphere light_hemisphere = 
        {
            .up_viewspace = {view_up[0], view_up[1], view_up[2]},
            .color_sky = {0.2f, 0.2f, 0.2f},
            .color_ground = {0.1f, 0.1f, 0.1f},
        };

        SDL_PushGPUFragmentUniformData(command_buffer_draw, 1, &light_hemisphere, sizeof(light_hemisphere));
    }

    SDL_PushGPUFragmentUniformData(command_buffer_draw, 2, &shadow_ubo, sizeof(shadow_ubo));
    
    Render_Unanimated(virtual_render_pass, command_buffer_draw);

    Render_BoneAnimated(virtual_render_pass, command_buffer_draw);

    Render_Sprite(virtual_render_pass, command_buffer_draw);

    // TODO need to overhaul text rendering
    if (text_renderable.vertex_buffer && text_renderable.index_buffer)
    {
        if (!Render_Text(virtual_render_pass, command_buffer_draw))
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to render text");
            return true;
        }
    }

    SDL_EndGPURenderPass(virtual_render_pass);

    SDL_GPUTexture* swapchain_texture;
    Uint32 swapchain_texture_width;
    Uint32 swapchain_texture_height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture
    (
        command_buffer_draw,
        window,
        &swapchain_texture,
        &swapchain_texture_width,
        &swapchain_texture_height
    ))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        // let's just skip the frame, but this may be a legitimate error
        // for e.g. a minimized window, SDL_WaitAndAcquireGPUSwapchainTexture should still return true, 
        // albeit with a NULL swapchain_texture (which is checked in the next `if` statement)
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    if (!swapchain_texture)
    {
        // Swapchain texture was NULL (e.g. minimized window) - skip drawing
        // No need to cancel the command buffer; just submit an empty one.
        SDL_SubmitGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    SDL_GPUColorTargetInfo swapchain_target_info = 
    {
        .texture = swapchain_texture,
        .clear_color = (SDL_FColor){ 1.0f, 1.0f, 1.0f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = true
    };

    SDL_GPURenderPass* swapchain_render_pass = SDL_BeginGPURenderPass
    (
        command_buffer_draw,
        &swapchain_target_info,
        1,
        NULL
    );

    if (!swapchain_render_pass)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin swapchain render pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    SDL_BindGPUGraphicsPipeline(swapchain_render_pass, pipeline_fullscreen_quad);

    mat4 projection_matrix_ortho;
    glm_ortho(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, projection_matrix_ortho);
    SDL_PushGPUVertexUniformData(command_buffer_draw, 0, &projection_matrix_ortho, sizeof(projection_matrix_ortho));

    SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &magic_debug, sizeof(magic_debug));

    SDL_GPUTextureSamplerBinding fullscreen_texture_binding = 
    { 
        .texture = virtual_screen_texture, 
        .sampler = default_texture_sampler 
    };

    if (magic_debug & MAGIC_DEBUG_SHADOW_DEPTH_TEXTURE)
    {
        fullscreen_texture_binding.texture = shadow_map_texture;
        fullscreen_texture_binding.sampler = shadow_sampler;
    }

    SDL_BindGPUFragmentSamplers
    (
        swapchain_render_pass, 
        0, // fragment sampler slot
        &fullscreen_texture_binding, 
        1 // num_bindings
    );

    SDL_DrawGPUPrimitives(swapchain_render_pass, 6, 1, 0, 0);

    SDL_EndGPURenderPass(swapchain_render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer_draw);

    return true;
}
