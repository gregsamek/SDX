#include "render.h"
#include "globals.h"
#include "text.h"
#include "sampler.h"
#include "pipeline.h"
#include "lights.h"

// INITIALIZATION /////////////////////////////////////////////////////////////

// TODO merge these settings with the settings_render flags?
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
                if (SDL_GPUTextureSupportsSampleCount(gpu_device, depth_texture_format, msaa_level) == false)
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Depth Texture does not support MSAA level %d on this device, using 1x MSAA", msaa);
                    msaa_level = SDL_GPU_SAMPLECOUNT_1;
                }
                if (SDL_GPUTextureSupportsSampleCount(gpu_device, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, msaa_level) == false)
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "MSAA level %d not supported for color textures on this device, using 1x MSAA", msaa);
                    msaa_level = SDL_GPU_SAMPLECOUNT_1;
                }
            }
            else if (SDL_strcmp(setting_name, "use_linear_filtering") == 0)
            {
                if (SDL_strtol(setting_value, NULL, 10))
                    Bit_Set(settings_render, SETTINGS_RENDER_USE_LINEAR_FILTERING);
                else
                    Bit_Clear(settings_render, SETTINGS_RENDER_USE_LINEAR_FILTERING);
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
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ
        }
    );
    if (virtual_screen_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create virtual screen texture: %s", SDL_GetError());
        return false;
    }

    if (prepass_texture_msaa)
    {
        SDL_ReleaseGPUTexture(gpu_device, prepass_texture_msaa);
    }
    prepass_texture_msaa = SDL_CreateGPUTexture
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
            .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
        }
    );
    if (prepass_texture_msaa == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create prepass msaa texture: %s", SDL_GetError());
        return false;
    }

    if (prepass_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, prepass_texture);
    }
    prepass_texture = SDL_CreateGPUTexture
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
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ
        }
    );
    if (prepass_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create prepass texture: %s", SDL_GetError());
        return false;
    }

    if (prepass_texture_half)
    {
        SDL_ReleaseGPUTexture(gpu_device, prepass_texture_half);
    }
    prepass_texture_half = SDL_CreateGPUTexture
    (
        gpu_device,
        &(SDL_GPUTextureCreateInfo)
        {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .width = virtual_screen_texture_width / 2,
            .height = virtual_screen_texture_height / 2,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE
        }
    );
    if (prepass_texture_half == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create prepass half res texture: %s", SDL_GetError());
        return false;
    }

    if (ssao_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, ssao_texture);
    }
    ssao_texture = SDL_CreateGPUTexture
    (
        gpu_device,
        &(SDL_GPUTextureCreateInfo)
        {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .width = virtual_screen_texture_width / 2,
            .height = virtual_screen_texture_height / 2,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .format = SDL_GPU_TEXTUREFORMAT_R16_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ
        }
    );
    if (ssao_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create SSAO texture: %s", SDL_GetError());
        return false;
    }

    if (ssao_texture_upsampled)
    {
        SDL_ReleaseGPUTexture(gpu_device, ssao_texture_upsampled);
    }
    ssao_texture_upsampled = SDL_CreateGPUTexture
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
            .format = SDL_GPU_TEXTUREFORMAT_R16_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE
        }
    );
    if (ssao_texture_upsampled == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create SSAO upsampled texture: %s", SDL_GetError());
        return false;
    }

    if (fog_texture)
    {
        SDL_ReleaseGPUTexture(gpu_device, fog_texture);
    }
    fog_texture = SDL_CreateGPUTexture
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
    if (fog_texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create prepass texture: %s", SDL_GetError());
        return false;
    }

    if (bloom_textures_downsampled[0])
    {
        for (int i = 0; i < MAX_BLOOM_LEVELS; i++)
        {
            SDL_ReleaseGPUTexture(gpu_device, bloom_textures_downsampled[i]);
            bloom_textures_downsampled[i] = NULL;
        }
    }
    for (int i = 0; i < MAX_BLOOM_LEVELS; i++)
    {
        bloom_textures_downsampled[i] = SDL_CreateGPUTexture
        (
            gpu_device,
            &(SDL_GPUTextureCreateInfo)
            {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .width = virtual_screen_texture_width >> (i + 1),
                .height = virtual_screen_texture_height >> (i + 1),
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE
            }
        );
        if (bloom_textures_downsampled[i] == NULL)
        {
            SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bloom level texture %d: %s", i, SDL_GetError());
            return false;
        }
    }

    if (bloom_textures_upsampled[0])
    {
        for (int i = 0; i < MAX_BLOOM_LEVELS; i++)
        {
            SDL_ReleaseGPUTexture(gpu_device, bloom_textures_upsampled[i]);
            bloom_textures_upsampled[i] = NULL;
        }
    }
    for (int i = 0; i < MAX_BLOOM_LEVELS; i++)
    {
        bloom_textures_upsampled[i] = SDL_CreateGPUTexture
        (
            gpu_device,
            &(SDL_GPUTextureCreateInfo)
            {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .width = virtual_screen_texture_width >> (i + 1),
                .height = virtual_screen_texture_height >> (i + 1),
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE
            }
        );
        if (bloom_textures_upsampled[i] == NULL)
        {
            SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create bloom level texture %d: %s", i, SDL_GetError());
            return false;
        }
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
			.format = depth_texture_format,
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
            .format = depth_sample_texture_format,
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
    shadow_settings = (Shadow_Settings){.texel_size = {1.0f / (float)SHADOW_MAP_SIZE, 1.0f / (float)SHADOW_MAP_SIZE}, .bias = SHADOW_BIAS, .pcf_radius = SHADOW_PCF_RADIUS};
    
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

    // TODO try for MAILBOX present mode
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
    
    if (!Sampler_Init())
    {
        return false;
    }
    
    if (!Render_InitRenderTargets())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize render targets");
        return false;
    }
    
    SDL_LogTrace(SDL_LOG_CATEGORY_GPU, "Renderer initialized successfully.");

    return true;
}

// FRAME RENDERING ////////////////////////////////////////////////////////////

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

static void Render_Unanimated_Prepass(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    if (!Array_Len(models_unanimated)) return;

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_prepass_unanimated);

    for (size_t i = 0; i < Array_Len(models_unanimated); i++)
    {
        // TODO implement model matrix per model
        mat4 model_matrix;
        glm_mat4_identity(model_matrix);

        mat4 mv_matrix;
        glm_mat4_mul(camera.view_matrix, model_matrix, mv_matrix);
        
        mat4 mvp_matrix;
        glm_mat4_mul(camera.view_projection_matrix, model_matrix, mvp_matrix);

        // TODO light mvp not needed for prepass; make a separate UBO without it?
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

        // need to sample diffuse because of alpha testing, otherwise depth buffer will be incorrect
        // if I get squeezed for performance, I could make a separate pipeline without alpha testing
        SDL_GPUTexture* texture_albedo = models_unanimated[i].mesh.material.texture_diffuse;    
        SDL_BindGPUFragmentSamplers
        (
            render_pass, 
            0, // first slot
            (SDL_GPUTextureSamplerBinding[])
            {
                { .texture = texture_albedo,  .sampler = sampler_albedo },
            },
            1 // num_bindings
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
                { .texture = texture_diffuse,  .sampler = sampler_albedo },
                { .texture = texture_metallic_roughness, .sampler = sampler_albedo },
                { .texture = texture_normal, .sampler = sampler_albedo }
            },
            3 // num_bindings
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
            (SDL_GPUTextureSamplerBinding[])
            {{
                .texture = models_bone_animated[i].model.mesh.material.texture_diffuse, 
                .sampler = sampler_albedo
            }}, 
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
            (SDL_GPUTextureSamplerBinding[])
            {{ 
                .texture = sequence->atlas_texture, 
                .sampler = sampler_albedo 
            }},
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
            (SDL_GPUTextureSamplerBinding[])
            {{
                .texture = sprites[i].texture, 
                .sampler = sampler_albedo 
            }}, 
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
        // foreach(model, models_unanimated)
        // {
        //     Model_Free(&model);
        // }
        // Array_Len(models_unanimated) = 0;
        // foreach(model_bone_animated, models_bone_animated)
        // {
        //     Model_BoneAnimated_Free(&model_bone_animated);
        // }
        // Array_Len(models_bone_animated) = 0;
        // if (!Model_Load_AllScenes())
        // {
        //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload models");
        //     return false;
        // }
        // foreach(sprite, sprites)
        // {
        //     SDL_ReleaseGPUTexture(gpu_device, sprite.texture);
        // }
        // Array_Len(sprites) = 0;
        // if (!Sprite_LoadSprites())
        // {
        //     SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload sprites");
        //     return false;
        // }
        // Array_Len(lights_spot) = 0;
        // if (!Lights_LoadLights())
        // {
        //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload lights");
        //     return false;
        // }
        if (!Render_Init())
        {
            return false;
        }
        renderer_needs_to_be_reinitialized = false;
    }

    if (Array_Len(models_bone_animated)) Model_JointMat_UpdateAndUpload();

    Lights_Update();

    SDL_GPUCommandBuffer* command_buffer_draw = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (command_buffer_draw == NULL)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }  

    ///////////////////////////////////////////////////////////////////////////
    // Shadow Pass ////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    
    if (settings_render & SETTINGS_RENDER_ENABLE_SHADOWS)
    {
        SDL_GPUDepthStencilTargetInfo shadow_target_info = 
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
            &shadow_target_info
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
        // Render_BoneAnimated_Shadow(shadow_pass, command_buffer_draw);

        SDL_EndGPURenderPass(shadow_pass);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Prepass ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////  

    SDL_GPUColorTargetInfo prepass_target_info = 
    {
        .texture = prepass_texture,
        .clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 0.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle_resolve_texture = true
    };

    if (msaa_level > SDL_GPU_SAMPLECOUNT_1)
    {
        prepass_target_info.texture = prepass_texture_msaa;
        prepass_target_info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
        prepass_target_info.resolve_texture = prepass_texture;
        prepass_target_info.cycle_resolve_texture = true;
    }

    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = 
    {
        .texture = depth_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = true,

        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
    };

    SDL_GPURenderPass* prepass_render_pass = SDL_BeginGPURenderPass
    (
        command_buffer_draw,
        &prepass_target_info,
        1,
        &depth_stencil_target_info
    );

    if (!prepass_render_pass)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin prepass render pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer_draw);
        return true;
    }
    
    SDL_SetGPUViewport(prepass_render_pass, &(SDL_GPUViewport)
    { 
        .x = 0, 
        .y = 0,
        .w = (int)virtual_screen_texture_width, 
        .h = (int)virtual_screen_texture_height,
        .min_depth = 0.0f, 
        .max_depth = 1.0f
    });

    Render_Unanimated_Prepass(prepass_render_pass, command_buffer_draw);

    SDL_EndGPURenderPass(prepass_render_pass);

    ///////////////////////////////////////////////////////////////////////////
    // Downsample Prepass /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    SDL_GPUComputePass* prepass_downsample_pass = SDL_BeginGPUComputePass
    (
        command_buffer_draw,
        (SDL_GPUStorageTextureReadWriteBinding[])
        {{
            .texture = prepass_texture_half,
            .cycle = true
        }},
        1,
        NULL,
        0
    );
    SDL_BindGPUComputePipeline(prepass_downsample_pass, pipeline_prepass_downsample);
    SDL_BindGPUComputeStorageTextures
    (
        prepass_downsample_pass,
        0, // first slot
        &prepass_texture,
        1 // num_bindings
    );
    SDL_DispatchGPUCompute(prepass_downsample_pass, virtual_screen_texture_width / 8, virtual_screen_texture_height / 8, 1);
    SDL_EndGPUComputePass(prepass_downsample_pass);
    
    ///////////////////////////////////////////////////////////////////////////
    // SSAO ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    
    if (settings_render & SETTINGS_RENDER_ENABLE_SSAO)
    {
        SDL_GPUColorTargetInfo ssao_target_info = 
        {
            .texture = ssao_texture,
            .clear_color = (SDL_FColor){ 1.0f, 1.0f, 1.0f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .cycle = true
        };

        SDL_GPURenderPass* ssao_render_pass = SDL_BeginGPURenderPass
        (
            command_buffer_draw,
            &ssao_target_info,
            1,
            NULL
        );

        if (!ssao_render_pass)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin SSAO render pass: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(command_buffer_draw);
            return true;
        }

        SDL_SetGPUViewport(ssao_render_pass, &(SDL_GPUViewport)
        { 
            .x = 0, 
            .y = 0,
            .w = (int)virtual_screen_texture_width / 2, 
            .h = (int)virtual_screen_texture_height / 2,
            .min_depth = 0.0f, 
            .max_depth = 1.0f
        });

        SDL_BindGPUGraphicsPipeline(ssao_render_pass, pipeline_ssao);

        SDL_BindGPUFragmentSamplers
        (
            ssao_render_pass,
            0, // first slot
            (SDL_GPUTextureSamplerBinding[])
            {
                { .texture = prepass_texture_half,  .sampler = sampler_nearest_nomips },
            },
            1 // num_bindings
        );

        UBO_SSAO ubo_ssao = 
        {
            .settings_render = settings_render,
            .screen_size = { (float)(virtual_screen_texture_width), (float)(virtual_screen_texture_height) },
            .radius = 0.5f,
            .bias = 0.01f,
            .intensity = 1.0f,
            .power = 1.0f,
            .kernel_size = 16.0f,
        };

        glm_mat4_copy(camera.projection_matrix, ubo_ssao.projection_matrix);

        SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &ubo_ssao, sizeof(ubo_ssao));

        SDL_DrawGPUPrimitives(ssao_render_pass, 6, 1, 0, 0);

        SDL_EndGPURenderPass(ssao_render_pass);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Upsample SSAO //////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    if ((settings_render & SETTINGS_RENDER_UPSCALE_SSAO) && (settings_render & SETTINGS_RENDER_ENABLE_SSAO))
    {
        SDL_GPUComputePass* ssao_upsample_pass = SDL_BeginGPUComputePass
        (
            command_buffer_draw,
            (SDL_GPUStorageTextureReadWriteBinding[])
            {{
                .texture = ssao_texture_upsampled,
                .cycle = true
            }},
            1,
            NULL,
            0
        );
        SDL_BindGPUComputePipeline(ssao_upsample_pass, pipeline_ssao_upsample);
        SDL_BindGPUComputeStorageTextures
        (
            ssao_upsample_pass,
            0, // first slot
            (SDL_GPUTexture*[])
            {
                ssao_texture,
                prepass_texture_half,
                prepass_texture
            },
            3 // num_bindings
        );
        UBO_SSAOUpsample ubo_ssao_upsample = 
        {
            .sigma_spatial = 1.0f,
            .sigma_depth = 1.0f,
            .sigma_normal = 0.1f,
            .normal_power = 8.0f
        };
        SDL_PushGPUComputeUniformData(command_buffer_draw, 0, &ubo_ssao_upsample, sizeof(ubo_ssao_upsample));
        SDL_DispatchGPUCompute(ssao_upsample_pass, virtual_screen_texture_width / 8, virtual_screen_texture_height / 8, 1);
        SDL_EndGPUComputePass(ssao_upsample_pass);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Main Pass //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    SDL_GPUColorTargetInfo virtual_target_info = 
    {
        .texture = virtual_screen_texture,
        .clear_color = (SDL_FColor){ 0.6f, 0.8f, 1.0f, 1.0f },
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

    depth_stencil_target_info = (SDL_GPUDepthStencilTargetInfo)
    {
        .texture = depth_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = false,

        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
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

    // can't skip these bindings, even if shadows & ssao are disabled
    // would need a separate pipeline if I wanted this for performance
    SDL_GPUTextureSamplerBinding ssao_binding;
    if (settings_render & SETTINGS_RENDER_UPSCALE_SSAO)
    {
        ssao_binding.texture = ssao_texture_upsampled;
        ssao_binding.sampler = sampler_nearest_nomips;
    }
    else
    {
        ssao_binding.texture = ssao_texture;
        ssao_binding.sampler = sampler_linear_nomips;
    }
    SDL_BindGPUFragmentSamplers
    (
        virtual_render_pass, 
        3, // first slot
        (SDL_GPUTextureSamplerBinding[])
        {
            { .texture = shadow_map_texture, .sampler = sampler_nearest_nomips },
            ssao_binding
        },
        2 // num_bindings
    );

    SDL_BindGPUFragmentStorageBuffers
    (
        virtual_render_pass,
        0, // storage buffer slot
        &lights_storage_buffer,
        1 // storage buffer count
    );

    UBO_Main_Frag ubo_main_frag =
    {
        .light_directional = light_directional,
        .light_hemisphere = light_hemisphere,
        .shadow_settings = shadow_settings,
        .inverse_screen_resolution = 
        {
            1.0f / (float)(virtual_screen_texture_width),
            1.0f / (float)(virtual_screen_texture_height)
        },
        .settings_render = settings_render,
        ._padding = 0.0f
    };

    SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &ubo_main_frag, sizeof(ubo_main_frag));

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

    ///////////////////////////////////////////////////////////////////////////
    // Fog Pass ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    if (settings_render & SETTINGS_RENDER_ENABLE_FOG)
    {
        SDL_GPUColorTargetInfo fog_target_info = 
        {
            .texture = fog_texture,
            .clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .store_op = SDL_GPU_STOREOP_STORE,
            .cycle = true
        };

        SDL_GPURenderPass* fog_render_pass = SDL_BeginGPURenderPass
        (
            command_buffer_draw,
            &fog_target_info,
            1,
            NULL
        );

        if (!fog_render_pass)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Failed to begin fog render pass: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(command_buffer_draw);
            return true;
        }

        SDL_BindGPUGraphicsPipeline(fog_render_pass, pipeline_fog);

        SDL_BindGPUFragmentSamplers
        (
            fog_render_pass, 
            0, // first slot
            (SDL_GPUTextureSamplerBinding[])
            {
                { .texture = virtual_screen_texture, .sampler = sampler_nearest_nomips },
                { .texture = prepass_texture, .sampler = sampler_nearest_nomips }
            },
            2 // num_bindings
        );

        UBO_Fog_Frag ubo_fog_frag =
        {
            .color = { 0.6f, 0.8f, 1.0f },
            .density = 0.01f,
            .start = 0.0f,
            .end = 100.0f,
            .mode = 2,
            .depth_is_view_z = 1,
            .height_fog_enable = 0.0f,
            .fog_height = 0.0f,
            .height_falloff = 0.0f,
        };
        glm_mat4_inv(camera.projection_matrix, ubo_fog_frag.inv_proj_mat);
        glm_mat4_inv(camera.view_matrix, ubo_fog_frag.inv_view_mat);

        SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &ubo_fog_frag, sizeof(ubo_fog_frag));

        SDL_DrawGPUPrimitives(fog_render_pass, 6, 1, 0, 0);

        SDL_EndGPURenderPass(fog_render_pass);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Bloom Pass /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // TODO slightly jitter the offsets per level (e.g., add a tiny blue-noise or alternating bias)

    if (settings_render & SETTINGS_RENDER_ENABLE_BLOOM)
    {
        // Threshold Pass /////////////////////////////////////////////////////

        SDL_GPUComputePass* bloom_threshold_pass = SDL_BeginGPUComputePass
        (
            command_buffer_draw,
            (SDL_GPUStorageTextureReadWriteBinding[])
            {{
                .texture = bloom_textures_downsampled[0],
                .cycle = true
            }},
            1,
            NULL,
            0
        );
        SDL_BindGPUComputePipeline(bloom_threshold_pass, pipeline_bloom_threshold);
        
        SDL_GPUTexture* texture_in = virtual_screen_texture;
        if (settings_render & SETTINGS_RENDER_ENABLE_FOG)
        {
            texture_in = fog_texture;
        }
        SDL_BindGPUComputeStorageTextures
        (
            bloom_threshold_pass,
            0, // first slot
            &texture_in,
            1 // num_bindings
        );
        
        UBO_Bloom_Threshold ubo_bloom_threshold = 
        {
            .threshold = 1.0f,
            .soft_knee = 0.5f,
            .use_maxRGB = 0,
            .exposure = 1.0f
        };
        SDL_PushGPUComputeUniformData(command_buffer_draw, 0, &ubo_bloom_threshold, sizeof(ubo_bloom_threshold));
        SDL_DispatchGPUCompute(bloom_threshold_pass, virtual_screen_texture_width / 8, virtual_screen_texture_height / 8, 1);
        SDL_EndGPUComputePass(bloom_threshold_pass);

        // Downsample Passes //////////////////////////////////////////////////

        for (int i = 1; i < MAX_BLOOM_LEVELS; i++)
        {
            SDL_GPUComputePass* bloom_downsample_pass = SDL_BeginGPUComputePass
            (
                command_buffer_draw,
                (SDL_GPUStorageTextureReadWriteBinding[])
                {{
                    .texture = bloom_textures_downsampled[i],
                    .cycle = true
                }},
                1,
                NULL,
                0
            );
            
            SDL_BindGPUComputePipeline(bloom_downsample_pass, pipeline_bloom_downsample);

            SDL_BindGPUComputeSamplers
            (
                bloom_downsample_pass,
                0, // first slot
                (SDL_GPUTextureSamplerBinding[])
                {
                    { .texture = bloom_textures_downsampled[i - 1],  .sampler = sampler_linear_nomips },
                },
                1 // num_bindings
            );

            UBO_Bloom_Sample ubo_bloom_downsample = 
            {
                .radius = (float)(i / 2 + 1), // TODO scale wrt resolution; if rendering at 360p, use 3 levels: [0.5, 0.5, 0.75]
                .tap_bias = (i & 1) * 0.5f,
            };
            SDL_PushGPUComputeUniformData(command_buffer_draw, 0, &ubo_bloom_downsample, sizeof(ubo_bloom_downsample));

            SDL_DispatchGPUCompute
            (
                bloom_downsample_pass,
                ((virtual_screen_texture_width >> i) + 7) / 8,
                ((virtual_screen_texture_height >> i) + 7) / 8,
                1
            );
            SDL_EndGPUComputePass(bloom_downsample_pass);
        }

        // Upsample Passes ////////////////////////////////////////////////////

        for (int i = MAX_BLOOM_LEVELS - 2; i >= 0; i--)
        {
            SDL_GPUComputePass* bloom_upsample_pass = SDL_BeginGPUComputePass
            (
                command_buffer_draw,
                (SDL_GPUStorageTextureReadWriteBinding[])
                {{
                    .texture = bloom_textures_upsampled[i],
                    .cycle = true
                }},
                1,
                NULL,
                0
            );
            
            SDL_BindGPUComputePipeline(bloom_upsample_pass, pipeline_bloom_upsample);

            SDL_BindGPUComputeSamplers
            (
                bloom_upsample_pass,
                0, // first slot
                (SDL_GPUTextureSamplerBinding[])
                {
                    { .texture = bloom_textures_downsampled[i + 1], .sampler = sampler_linear_nomips },
                    { .texture = bloom_textures_downsampled[i],     .sampler = sampler_linear_nomips }
                },
                2 // num_bindings
            );

            UBO_Bloom_Sample ubo_bloom_upsample = 
            {
                .radius = (float)(i / 2 + 1), // TODO scale wrt resolution
                .tap_bias = (i & 1) * 0.5f
            };
            SDL_PushGPUComputeUniformData(command_buffer_draw, 0, &ubo_bloom_upsample, sizeof(ubo_bloom_upsample));

            SDL_DispatchGPUCompute
            (
                bloom_upsample_pass,
                ((virtual_screen_texture_width >> i) + 7) / 8,
                ((virtual_screen_texture_height >> i) + 7) / 8,
                1
            );
            SDL_EndGPUComputePass(bloom_upsample_pass);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Swapchain Pass /////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

    SDL_BindGPUGraphicsPipeline(swapchain_render_pass, pipeline_swapchain);

    SDL_PushGPUFragmentUniformData(command_buffer_draw, 0, &settings_render, sizeof(settings_render));

    SDL_GPUTextureSamplerBinding fullscreen_texture_binding = 
    { 
        .texture = virtual_screen_texture, 
        .sampler = sampler_albedo 
    };

    if (settings_render & SETTINGS_RENDER_ENABLE_FOG)
    {
        fullscreen_texture_binding.texture = fog_texture;
    }

    if (settings_render & SETTINGS_RENDER_SHOW_DEBUG_TEXTURE)
    {
        fullscreen_texture_binding.texture = bloom_textures_upsampled[0];
        fullscreen_texture_binding.sampler = sampler_nearest_nomips;
    }

    SDL_BindGPUFragmentSamplers
    (
        swapchain_render_pass, 
        0, // fragment sampler slot
        (SDL_GPUTextureSamplerBinding[])
        {
            fullscreen_texture_binding,
            { .texture = bloom_textures_upsampled[0],  .sampler = sampler_linear_nomips },
        }, 
        2 // num_bindings
    );

    SDL_DrawGPUPrimitives(swapchain_render_pass, 6, 1, 0, 0);

    SDL_EndGPURenderPass(swapchain_render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer_draw);

    return true;
}
