#include <SDL3/SDL.h>

#include "pipeline.h"
#include "globals.h"
#include "text.h"
#include "shader.h"

bool Pipeline_Init()
{
    if (!Pipeline_Prepass_Unanimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize prepass unanimated pipeline!");
        return false;
    }
    if (!Pipeline_SSAO_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize SSAO pipeline!");
        return false;
    }
    if (!Pipeline_Unlit_Unanimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize unanimated pipeline!");
        return false;
    }
    if (!Pipeline_BlinnPhong_Unanimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize unanimated phong pipeline!");
        return false;
    }
    if (!Pipeline_PBR_Unanimated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize unanimated PBR pipeline!");
        return false;
    }
    // if (!Pipeline_PBR_Animated_Init())
    // {
    //     SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bone animated pipeline!");
    //     return false;
    // }
    if (!Pipeline_Text_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize text pipeline!");
        return false;
    }
    if (!Pipeline_Swapchain_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize fullscreen quad pipeline!");
        return false;
    }
    if (!Pipeline_Sprite_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize sprite pipeline!");
        return false;
    }
    if (!Pipeline_ShadowDepth_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize shadow depth pipeline!");
        return false;
    }
    if (!Pipeline_Fog_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize fog pipeline!");
        return false;
    }
    if (!Pipeline_PrepassDownsample_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize prepass downsample compute pipeline!");
        return false;
    }
    if (!Pipeline_SSAOUpsample_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize SSAO upsample compute pipeline!");
        return false;
    }
    if (!Pipeline_Bloom_Threshold_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom threshold compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Prepass_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "prepass_unanimated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers (transform matrices)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "prepass.frag", // Base filename
        1, // num_samplers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = false,
                    // .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = false
                }
            }},
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_sample_texture_format
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
                    .pitch = sizeof(Vertex_PBR) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 4,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position: TEXCOORD0
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0,
                    .offset = offsetof(Vertex_PBR, x)
                },
                {   // normal: TEXCOORD1
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 1,
                    .offset = offsetof(Vertex_PBR, nx)
                },
                {   // texture coordinate: TEXCOORD2
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 2,
                    .offset = offsetof(Vertex_PBR, u)
                },
                {
                    // tangent: TEXCOORD3
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                    .location = 3,
                    .offset = offsetof(Vertex_PBR, tx)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };
    if (pipeline_prepass_unanimated)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_prepass_unanimated);
        pipeline_prepass_unanimated = NULL;
    }
    pipeline_prepass_unanimated = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_prepass_unanimated == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_SSAO_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "fullscreen_quad.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }
    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "ssao.frag", // Base filename
        1, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16_FLOAT,
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = false,
                    // .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = false
                }
            }},
            .has_depth_stencil_target = false,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = SDL_GPU_SAMPLECOUNT_1 }
    };
    if (pipeline_ssao)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_ssao);
        pipeline_ssao = NULL;
    }
    pipeline_ssao = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_ssao == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_Unlit_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "unlit_unanimated.vert", // Base filename
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
        "unlit_alphatest.frag", // Base filename
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
    if (pipeline_unanimated)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_unanimated);
        pipeline_unanimated = NULL;
    }
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

bool Pipeline_BlinnPhong_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "blinnphong_unanimated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers (transform matrices)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "blinnphong_alphatest.frag", // Base filename
        3, // num_samplers
        0, // num_storage_textures
        1, // num_storage_buffers
        1  // num_uniform_buffers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
            .enable_depth_write = false,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_EQUAL,
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
                    .pitch = sizeof(Vertex_PBR) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 4,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position: TEXCOORD0
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0,
                    .offset = offsetof(Vertex_PBR, x)
                },
                {   // normal: TEXCOORD1
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 1,
                    .offset = offsetof(Vertex_PBR, nx)
                },
                {   // texture coordinate: TEXCOORD2
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 2,
                    .offset = offsetof(Vertex_PBR, u)
                },
                {
                    // tangent: TEXCOORD3
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                    .location = 3,
                    .offset = offsetof(Vertex_PBR, tx)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };
    if (pipeline_unanimated)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_unanimated);
        pipeline_unanimated = NULL;
    }
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

bool Pipeline_PBR_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "pbr_unanimated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers (transform matrices)
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "pbr_alphatest.frag", // Base filename
        5, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1 // num_uniform_buffers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
            .enable_depth_write = false,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_EQUAL,
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
                    .pitch = sizeof(Vertex_PBR) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 4,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position: TEXCOORD0
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0,
                    .offset = offsetof(Vertex_PBR, x)
                },
                {   // normal: TEXCOORD1
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 1,
                    .offset = offsetof(Vertex_PBR, nx)
                },
                {   // texture coordinate: TEXCOORD2
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 2,
                    .offset = offsetof(Vertex_PBR, u)
                },
                {
                    // tangent: TEXCOORD3
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                    .location = 3,
                    .offset = offsetof(Vertex_PBR, tx)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };
    if (pipeline_unanimated)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_unanimated);
        pipeline_unanimated = NULL;
    }
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

bool Pipeline_PBR_Animated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "pbr_animated.vert", // Base filename
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
        "pbr_alphatest.frag", // Base filename
        3, // num_samplers (for the single texture)
        0, // num_storage_textures
        1, // num_storage_buffers
        1  // num_uniform_buffers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
            .enable_depth_write = false,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_EQUAL,
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
            .num_vertex_attributes = 6,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0, // TEXCOORD0 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, x)
                },
                {   // normal
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 1, // TEXCOORD1 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, nx)
                },
                {   // texture coordinate
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 2, // TEXCOORD2 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, u)
                },
                {
                    // tangent: TEXCOORD3
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                    .location = 3,
                    .offset = offsetof(Vertex_BoneAnimated, tx)
                },
                {   // joint IDs
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT, // in the shader this is interpreted as Uint8[4]
                    .location = 4, // TEXCOORD4 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, joint_ids)
                },
                {   // joint weights
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, // vec4 Float32
                    .location = 5, // TEXCOORD5 in HLSL
                    .offset = offsetof(Vertex_BoneAnimated, weights)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };
    if (pipeline_bone_animated)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_bone_animated);
        pipeline_bone_animated = NULL;
    }
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
    if (pipeline_text)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_text);
        pipeline_text = NULL;
    }
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

// This is the pipeline that renders the "virtual screen" quad to the swapchain
bool Pipeline_Swapchain_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "fullscreen_quad.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "swapchain.frag", // Base filename
        1, // num_samplers (for the single texture)
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers
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
            .has_depth_stencil_target = false,
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = SDL_GPU_SAMPLECOUNT_1 }
    };
    if (pipeline_fullscreen_quad)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_fullscreen_quad);
        pipeline_fullscreen_quad = NULL;
    }
    pipeline_fullscreen_quad = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_fullscreen_quad == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_Sprite_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "sprite.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "unlit_alphatest.frag", // Base filename
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
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
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = msaa_level }
    };
    if (pipeline_sprite)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_sprite);
        pipeline_sprite = NULL;
    }
    pipeline_sprite = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_sprite == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_ShadowDepth_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "shadow_unanimated.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    // TODO fragment shader cannot be NULL, need to create a minimal shader that matches the input of the vertex shader
    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "shadow.frag", // Base filename
        0, // num_samplers
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
            .num_color_targets = 0,
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_sample_texture_format
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
            .cull_mode = SDL_GPU_CULLMODE_BACK, // TODO consider front face culling for shadow maps if peter panning becomes an issue
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f, // 1.25f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 1.75f,
            .enable_depth_bias = true,
            .enable_depth_clip = true
        },
        .vertex_input_state = (SDL_GPUVertexInputState)
        {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[])
            {
                {
                    .slot = 0,
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .pitch = sizeof(Vertex_PBR) // MUST MATCH LOADED VERTEX DATA
                }
            },  
            .num_vertex_attributes = 4,
            .vertex_attributes = (SDL_GPUVertexAttribute[])
            {
                {   // position: TEXCOORD0
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 0,
                    .offset = offsetof(Vertex_PBR, x)
                },
                {   // normal: TEXCOORD1
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                    .location = 1,
                    .offset = offsetof(Vertex_PBR, nx)
                },
                {   // texture coordinate: TEXCOORD2
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .location = 2,
                    .offset = offsetof(Vertex_PBR, u)
                },
                {
                    // tangent: TEXCOORD3
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                    .location = 3,
                    .offset = offsetof(Vertex_PBR, tx)
                }
            }
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = SDL_GPU_SAMPLECOUNT_1 }
    };
    if (pipeline_shadow_depth)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_shadow_depth);
        pipeline_shadow_depth = NULL;
    }
    pipeline_shadow_depth = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_shadow_depth == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);

    return true;
}

bool Pipeline_Fog_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "fullscreen_quad.vert", // Base filename
        0, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        0  // num_uniform_buffers
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }
    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "fog.frag", // Base filename
        2, // num_samplers
        0, // num_storage_textures
        0, // num_storage_buffers
        1  // num_uniform_buffers
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
                .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                .blend_state = (SDL_GPUColorTargetBlendState)
                {
                    .enable_blend = false,
                    // .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    // .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    // .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    // .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                    .enable_color_write_mask = false
                }
            }},
            .has_depth_stencil_target = false,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID
        },
        .rasterizer_state = (SDL_GPURasterizerState)
        {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .multisample_state = (SDL_GPUMultisampleState) { .sample_count = SDL_GPU_SAMPLECOUNT_1 }
    };
    if (pipeline_fog)
    {
        SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline_fog);
        pipeline_fog = NULL;
    }
    pipeline_fog = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_create_info);
    if (pipeline_fog == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    return true;
}

bool Pipeline_PrepassDownsample_Init()
{
    if (pipeline_prepass_downsample)
    {
        SDL_ReleaseGPUComputePipeline(gpu_device, pipeline_prepass_downsample);
        pipeline_prepass_downsample = NULL;
    }
    pipeline_prepass_downsample = Pipeline_Compute_Init
    (
        gpu_device,"prepass_downsample.comp",
        &(SDL_GPUComputePipelineCreateInfo)
        {
			.num_readonly_storage_textures = 1,
			.num_readwrite_storage_textures = 1,
			.threadcount_x = 8,
			.threadcount_y = 8,
			.threadcount_z = 1,
        }
    );
    if (pipeline_prepass_downsample == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize prepass downsample compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_SSAOUpsample_Init()
{
    if (pipeline_ssao_upsample)
    {
        SDL_ReleaseGPUComputePipeline(gpu_device, pipeline_ssao_upsample);
        pipeline_ssao_upsample = NULL;
    }
    pipeline_ssao_upsample = Pipeline_Compute_Init
    (
        gpu_device,"ssao_upsample.comp",
        &(SDL_GPUComputePipelineCreateInfo)
        {
			.num_readonly_storage_textures = 3,
			.num_readwrite_storage_textures = 1,
            .num_uniform_buffers = 1,
			.threadcount_x = 8,
			.threadcount_y = 8,
			.threadcount_z = 1,
        }
    );
    if (pipeline_ssao_upsample == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize SSAO upsample compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Bloom_Threshold_Init()
{
    if (pipeline_bloom_threshold)
    {
        SDL_ReleaseGPUComputePipeline(gpu_device, pipeline_bloom_threshold);
        pipeline_bloom_threshold = NULL;
    }
    pipeline_bloom_threshold = Pipeline_Compute_Init
    (
        gpu_device,"bloom_threshold.comp",
        &(SDL_GPUComputePipelineCreateInfo)
        {
            .num_readonly_storage_textures = 1,
            .num_readwrite_storage_textures = 1,
            .num_uniform_buffers = 1,
            .threadcount_x = 8,
            .threadcount_y = 8,
            .threadcount_z = 1,
        }
    );
    if (pipeline_bloom_threshold == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom threshold compute pipeline!");
        return false;
    }
    return true;
}

SDL_GPUComputePipeline* Pipeline_Compute_Init
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
) 
{
	char fullPath[MAXIMUM_URI_LENGTH];
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