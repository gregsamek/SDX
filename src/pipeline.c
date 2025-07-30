#include <SDL3/SDL.h>

#include "pipeline.h"
#include "globals.h"
#include "text.h"
#include "shader.h"

bool Pipeline_Init()
{
    if (!Pipeline_Unanimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize unanimated pipeline!");
        return false;
    }
    if (!Pipeline_BoneAnimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bone animated pipeline!");
        return false;
    }
    if (!Pipeline_Text_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize text pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "unanimated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers (MVP matrix)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "unanimated.frag", // Base filename
        1, // num_samplers (for the single texture)
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers
    );
    if (fragment_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load fragment shader!");
        return false;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info =
    {
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = true,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = true
                }
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_texture_format
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState)
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .vertex_input_state = (SDL_GPUVertexInputState)
        {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {
                {
                    .slot = 0,
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .pitch = sizeof(Vertex_PositionTexture) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 2,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0, // TEXCOORD0 in HLSL
                    .offset = offsetof(Vertex_PositionTexture, x)
                },
                {   // texture coordinate
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 1, // TEXCOORD1 in HLSL
                    .offset = offsetof(Vertex_PositionTexture, u)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };

    pipeline_unanimated = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_unanimated == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_BoneAnimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "bone_animated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        1, // num_storage_buffers (joint matrices)
        2  // num_uniform_buffers (MVP matrix, joint matrices storage buffer offset)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "unanimated.frag", // Base filename
        1, // num_samplers (for the single texture)
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers
    );
    if (fragment_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load fragment shader!");
        return false;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info =
    {
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = true,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = true
                }
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_texture_format
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState)
        {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .vertex_input_state = (SDL_GPUVertexInputState)
        {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {
                {
                    .slot = 0,
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .pitch = sizeof(Vertex_BoneAnimated) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 4,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0, // TEXCOORD0 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, x)
                },
                {   // texture coordinate
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 1, // TEXCOORD1 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, u)
                },
                {   // joint IDs
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT, // in the shader this is interpreted as Uint8[4]
                    .location = 2, // TEXCOORD2 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, joint_ids)
                },
                {   // joint weights
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, // vec4 Float32
                    .location = 3, // TEXCOORD3 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, weights)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };

    pipeline_bone_animated = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_bone_animated == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_RigidAnimated_Init()
{
    // TODO
    return false;
}

bool Pipeline_Instanced_Init()
{
    // TODO
    return false;
}

bool Pipeline_Text_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "text.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers (screen size)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "text.frag", // Base filename
        1, // num_samplers (for the font texture)
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers (text color?)
    );
    if (fragment_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load fragment shader!");
        return false;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info =
    {
        .target_info =
        {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[])
            {{
                .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = true,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = true
                }
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_texture_format
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState)
        {
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .vertex_input_state = (SDL_GPUVertexInputState)
        {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {
                {
                    .slot = 0,
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .pitch = sizeof(Text_Vertex) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 2,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 0, // TEXCOORD0 in HLSL
                    .offset = offsetof(Text_Vertex, xy)
                },
                {   // texture coordinate
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 1, // TEXCOORD1 in HLSL
                    .offset = offsetof(Text_Vertex, uv)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };

    pipeline_text = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_text == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

SDL_GPUComputePipeline* Pipeline_Compute_Init
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
) 
{
	char fullPath[512];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(gpu_device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.spv", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} 
    else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.msl", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} 
    else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) 
    {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/%s.dxil", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} 
    else 
    {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", fullPath);
		return NULL;
	}

	// Make a copy of the create data, then overwrite the parts we need
	SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
	newCreateInfo.code = (const Uint8*) code;
	newCreateInfo.code_size = codeSize;
	newCreateInfo.entrypoint = entrypoint;
	newCreateInfo.format = format;

	SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(gpu_device, &newCreateInfo);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create compute pipeline!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return pipeline;
}