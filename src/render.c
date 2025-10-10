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

bool Render_Init()
{
    SDL_WaitForGPUIdle(gpu_device);

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

static void Render_Unanimated(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!models_unanimated.len) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_unanimated);

    SDL_BindGPUVertexStorageBuffers
    (
        render_pass,
        0, // storage buffer slot
        &lights_storage_buffer,
        1 // storage buffer count
    );

    for (size_t i = 0; i < models_unanimated.len; i++)
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

        SDL_PushGPUVertexUniformData(command_buffer, 0, &transforms, sizeof(transforms));

        vec3 light_position_world = {0.0f, 5.0f, 0.0f};
        vec3 light_color = {1.0f, 1.0f, 1.0f};
        float ambient_strength = 0.1f;
        
        vec4 lp_world4 = { light_position_world[0], light_position_world[1], light_position_world[2], 1.0f };
        vec4 lp_view4;
        glm_mat4_mulv(camera.view_matrix, lp_world4, lp_view4);
        vec3 light_pos_vs = { lp_view4[0], lp_view4[1], lp_view4[2] };

        Light_Point lighting = {0};
        lighting.position[0] = light_pos_vs[0];
        lighting.position[1] = light_pos_vs[1];
        lighting.position[2] = light_pos_vs[2];
        lighting.color[0] = light_color[0];
        lighting.color[1] = light_color[1];
        lighting.color[2] = light_color[2];

        SDL_PushGPUFragmentUniformData(command_buffer, 0, &lighting, sizeof(lighting));
        
        SDL_BindGPUVertexBuffers
        (
            render_pass, 
            0, // vertex buffer slot
            (SDL_GPUBufferBinding[])
            {
                { 
                    .buffer = models_unanimated.arr[i].vertex_buffer, 
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
                .buffer = models_unanimated.arr[i].index_buffer, 
                .offset = 0 
            }, 
            SDL_GPU_INDEXELEMENTSIZE_16BIT
        );

        SDL_GPUTexture* diffuse_tex = models_unanimated.arr[i].texture;    
        SDL_GPUTexture* specular_tex = models_unanimated.arr[i].texture;
        
        SDL_BindGPUFragmentSamplers
        (
            render_pass, 
            0, // first slot
            (SDL_GPUTextureSamplerBinding[])
            {
                { .texture = diffuse_tex,  .sampler = default_texture_sampler },
                { .texture = specular_tex, .sampler = default_texture_sampler }
            },
            2 // num_bindings
        );

        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            (Uint32)models_unanimated.arr[i].index_count, // num_indices
            1,  // num_instances
            0,  // first_index
            0,  // vertex_offset
            0   // first_instance
        );
    }
}

static void Render_BoneAnimated(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!models_bone_animated.len) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_bone_animated);
    SDL_BindGPUVertexStorageBuffers
    (
        render_pass,
        0, // storage buffer slot
        &joint_matrix_storage_buffer,
        1 // storage buffer count
    );

    for (size_t i = 0; i < models_bone_animated.len; i++)
    {
        mat4 mvp_matrix;
        glm_mat4_mul(camera.view_projection_matrix, models_bone_animated.arr[i].model_matrix, mvp_matrix);
        // glm_mat4_mul(projection_matrix_ortho, models_bone_animated.arr[i].model_matrix, mvp_matrix);
        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp_matrix, sizeof(mvp_matrix));

        SDL_PushGPUVertexUniformData
        (
            command_buffer, 
            1, 
            &models_bone_animated.arr[i].storage_buffer_offset_bytes, 
            sizeof(Uint32)
        );

        SDL_BindGPUVertexBuffers
        (
            render_pass, 
            0, // vertex buffer slot
            (SDL_GPUBufferBinding[])
            {
                { 
                    .buffer = models_bone_animated.arr[i].vertex_buffer, 
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
                .buffer = models_bone_animated.arr[i].index_buffer, 
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
                .texture = models_bone_animated.arr[i].texture, 
                .sampler = default_texture_sampler 
            }, 
            1 // num_bindings
        );

        SDL_DrawGPUIndexedPrimitives
        (
            render_pass,
            (Uint32)models_bone_animated.arr[i].index_count, // num_indices
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
    if (!sprites.len) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_sprite);

    for (size_t i = 0; i < sprites.len; i++)
    {
        float sprite_height = sprites.arr[i].height;
        float sprite_width = sprite_height * sprites.arr[i].aspect_ratio * virtual_screen_texture_height / (float)virtual_screen_texture_width;
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
                .texture = sprites.arr[i].texture, 
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
        if (!Model_LoadAllModels())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload models");
            return false;
        }
        if (!Sprite_LoadSprites())
        {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload sprites");
            return false;
        }
        if (!Render_Init())
        {
            return false;
        }
        renderer_needs_to_be_reinitialized = false;
    }

    if (models_bone_animated.len) Model_JointMat_UpdateAndUpload();

    SDL_GPUCommandBuffer* command_buffer_draw = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (command_buffer_draw == NULL)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUColorTargetInfo virtual_target_info = 
    {
        .texture = virtual_screen_texture,
        .clear_color = (SDL_FColor){ 1.0f, 1.0f, 1.0f, 1.0f },
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
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin render pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    SDL_SetGPUViewport
    (
        virtual_render_pass, 
        &(SDL_GPUViewport)
        { 
            .x = 0, 
            .y = 0,
            .w = (int)virtual_screen_texture_width, 
            .h = (int)virtual_screen_texture_height,
            .min_depth = 0.0f, 
            .max_depth = 1.0f
        }
    );
    
    Render_Unanimated(virtual_render_pass, command_buffer_draw);

    Render_BoneAnimated(virtual_render_pass, command_buffer_draw);

    // TODO need to overhaul text rendering
    if (text_renderable.vertex_buffer && text_renderable.index_buffer)
    {
        if (!Render_Text(virtual_render_pass, command_buffer_draw))
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to render text");
            return true;
        }
    }

    Render_Sprite(virtual_render_pass, command_buffer_draw);

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
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin render pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }

    SDL_BindGPUGraphicsPipeline(swapchain_render_pass, pipeline_fullscreen_quad);

    mat4 projection_matrix_ortho;
    glm_ortho(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, projection_matrix_ortho);
    SDL_PushGPUVertexUniformData(command_buffer_draw, 0, &projection_matrix_ortho, sizeof(projection_matrix_ortho));

    SDL_BindGPUFragmentSamplers
    (
        swapchain_render_pass, 
        0, // fragment sampler slot
        &(SDL_GPUTextureSamplerBinding)
        { 
            .texture = virtual_screen_texture, 
            .sampler = default_texture_sampler 
        }, 
        1 // num_bindings
    );

    SDL_DrawGPUPrimitives(swapchain_render_pass, 6, 1, 0, 0);

    SDL_EndGPURenderPass(swapchain_render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer_draw);

    return true;
}
