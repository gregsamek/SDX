#include <SDL3/SDL.h>

#include "pipeline.h"
#include "globals.h"
#include "text.h"
#include "json.h"

static SDL_GPUShader* Shader_Load (SDL_GPUDevice* gpu_device, const char* shaderFilename);
static SDL_GPUComputePipeline* Pipeline_Compute_Init (SDL_GPUDevice* gpu_device, const char* shaderFilename);

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
    if (!Pipeline_PBR_Animated_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bone animated pipeline!");
        return false;
    }
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
    if (!Pipeline_Bloom_Downsample_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom downsample compute pipeline!");
        return false;
    }
    if (!Pipeline_Bloom_Upsample_Init())
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom upsample compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Prepass_Unanimated_Init()
{
    SDL_GPUShader* vertex_shader = Shader_Load
    (
        gpu_device,
        "prepass_unanimated.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "prepass.frag"
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
        "fullscreen_quad.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }
    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "ssao.frag"
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
        "unlit_unanimated.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "unlit_alphatest.frag"
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
        "blinnphong_unanimated.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "blinnphong_alphatest.frag"
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
        "pbr_unanimated.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "pbr_alphatest.frag"
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
        "pbr_animated.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "pbr_alphatest.frag"
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
        "text.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "text.frag"
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
        "fullscreen_quad.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "swapchain.frag"
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
        "sprite.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }

    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "unlit_alphatest.frag"
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
        "shadow_unanimated.vert"
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
        "shadow.frag"
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
        "fullscreen_quad.vert"
    );
    if (vertex_shader == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader!");
        return false;
    }
    SDL_GPUShader* fragment_shader = Shader_Load
    (
        gpu_device,
        "fog.frag"
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
        gpu_device,"prepass_downsample.comp"
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
        gpu_device,"ssao_upsample.comp"
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
        gpu_device,"bloom_threshold.comp"
    );
    if (pipeline_bloom_threshold == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom threshold compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Bloom_Downsample_Init()
{
    if (pipeline_bloom_downsample)
    {
        SDL_ReleaseGPUComputePipeline(gpu_device, pipeline_bloom_downsample);
        pipeline_bloom_downsample = NULL;
    }
    pipeline_bloom_downsample = Pipeline_Compute_Init
    (
        gpu_device,"bloom_downsample.comp"
    );
    if (pipeline_bloom_downsample == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom downsample compute pipeline!");
        return false;
    }
    return true;
}

bool Pipeline_Bloom_Upsample_Init()
{
    if (pipeline_bloom_upsample)
    {
        SDL_ReleaseGPUComputePipeline(gpu_device, pipeline_bloom_upsample);
        pipeline_bloom_upsample = NULL;
    }
    pipeline_bloom_upsample = Pipeline_Compute_Init
    (
        gpu_device,"bloom_upsample.comp"
    );
    if (pipeline_bloom_upsample == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to initialize bloom upsample compute pipeline!");
        return false;
    }
    return true;
}

static SDL_GPUComputePipeline* Pipeline_Compute_Init
(
	SDL_GPUDevice* gpu_device,
	const char* shaderFilename
) 
{
	char full_path[MAXIMUM_URI_LENGTH];
	SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu_device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.spv", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} 
    else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.msl", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} 
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.dxil", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} 
    else 
    {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t code_size;
	void* code = SDL_LoadFile(full_path, &code_size);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", full_path);
		return NULL;
	}

	SDL_GPUComputePipelineCreateInfo create_info = 
    {
        .code = (const Uint8*) code,
        .code_size = code_size,
        .entrypoint = entrypoint,
        .format = format,
    };

    // Shader Reflection //////////////////////////////////////////////////////

    char json_path[MAXIMUM_URI_LENGTH];
    SDL_snprintf(json_path, sizeof(json_path), "%sshaders/%s.json", base_path, shaderFilename);
    size_t json_size;
    char* json_raw = (char*)SDL_LoadFile(json_path, &json_size);
    if (json_raw == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load compute shader reflection JSON from disk! %s", json_path);
        SDL_free(code);
        return NULL;
    }

    cJSON* json = cJSON_ParseWithLength(json_raw, (int)json_size);
    if (json == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to parse compute shader reflection JSON!");
        SDL_free(code);
        SDL_free(json_raw);
        return NULL;
    }

    cJSON* num_samplers_json = cJSON_GetObjectItemCaseSensitive(json, "samplers");
    if (cJSON_IsNumber(num_samplers_json))
    {
        create_info.num_samplers = cJSON_GetNumberValue(num_samplers_json);
    }

    cJSON* num_readonly_storage_textures_json = cJSON_GetObjectItemCaseSensitive(json, "readonly_storage_textures");
    if (cJSON_IsNumber(num_readonly_storage_textures_json))
    {
        create_info.num_readonly_storage_textures = cJSON_GetNumberValue(num_readonly_storage_textures_json);
    }

    cJSON* num_readonly_storage_buffers_json = cJSON_GetObjectItemCaseSensitive(json, "readonly_storage_buffers");
    if (cJSON_IsNumber(num_readonly_storage_buffers_json))
    {
        create_info.num_readonly_storage_buffers = cJSON_GetNumberValue(num_readonly_storage_buffers_json);
    }

    cJSON* num_readwrite_storage_textures_json = cJSON_GetObjectItemCaseSensitive(json, "readwrite_storage_textures");
    if (cJSON_IsNumber(num_readwrite_storage_textures_json))
    {
        create_info.num_readwrite_storage_textures = cJSON_GetNumberValue(num_readwrite_storage_textures_json);
    }

    cJSON* num_readwrite_storage_buffers_json = cJSON_GetObjectItemCaseSensitive(json, "readwrite_storage_buffers");
    if (cJSON_IsNumber(num_readwrite_storage_buffers_json))
    {
        create_info.num_readwrite_storage_buffers = cJSON_GetNumberValue(num_readwrite_storage_buffers_json);
    }

    cJSON* num_uniform_buffers_json = cJSON_GetObjectItemCaseSensitive(json, "uniform_buffers");
    if (cJSON_IsNumber(num_uniform_buffers_json))
    {
        create_info.num_uniform_buffers = cJSON_GetNumberValue(num_uniform_buffers_json);
    }

    cJSON* threadcount_x_json = cJSON_GetObjectItemCaseSensitive(json, "threadcount_x");
    if (cJSON_IsNumber(threadcount_x_json))
    {
        create_info.threadcount_x = cJSON_GetNumberValue(threadcount_x_json);
    }

    cJSON* threadcount_y_json = cJSON_GetObjectItemCaseSensitive(json, "threadcount_y");
    if (cJSON_IsNumber(threadcount_y_json))
    {
        create_info.threadcount_y = cJSON_GetNumberValue(threadcount_y_json);
    }

    cJSON* threadcount_z_json = cJSON_GetObjectItemCaseSensitive(json, "threadcount_z");
    if (cJSON_IsNumber(threadcount_z_json))
    {
        create_info.threadcount_z = cJSON_GetNumberValue(threadcount_z_json);
    }

    cJSON_Delete(json);
    SDL_free(json_raw);

	SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(gpu_device, &create_info);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create compute pipeline!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return pipeline;
}

static SDL_GPUShader* Shader_Load (SDL_GPUDevice* gpu_device, const char* shaderFilename) 
{
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Invalid shader stage: %s", shaderFilename);
		return NULL;
	}

	char full_path[MAXIMUM_URI_LENGTH];
	SDL_GPUShaderFormat supported_shader_formats = SDL_GetGPUShaderFormats(gpu_device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (supported_shader_formats & SDL_GPU_SHADERFORMAT_SPIRV) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.spv", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} 
    else if (supported_shader_formats & SDL_GPU_SHADERFORMAT_MSL) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.msl", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} 
    else if (supported_shader_formats & SDL_GPU_SHADERFORMAT_DXIL) 
    {
		SDL_snprintf(full_path, sizeof(full_path), "%sshaders/%s.dxil", base_path, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} 
    else 
    {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No valid backend shader format (SPIRV, MSL, DXIL)");
		return NULL;
	}

	size_t code_size;
	void* code = SDL_LoadFile(full_path, &code_size);
	if (code == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load shader from disk! %s", full_path);
		return NULL;
	}

    // Shader Reflection //////////////////////////////////////////////////////
	
    char json_path[MAXIMUM_URI_LENGTH];
	SDL_snprintf(json_path, sizeof(json_path), "%sshaders/%s.json", base_path, shaderFilename);
	size_t json_size;
	char* json_raw = (char*)SDL_LoadFile(json_path, &json_size);
	if (json_raw == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load shader reflection JSON from disk! %s", json_path);
		SDL_free(code);
		return NULL;
	}

	cJSON* json = cJSON_ParseWithLength(json_raw, (int)json_size);
	if (json == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to parse shader reflection JSON!");
		SDL_free(code);
		SDL_free(json_raw);
		return NULL;
	}

	cJSON* num_samplers_json = cJSON_GetObjectItemCaseSensitive(json, "samplers");
	Uint32 num_samplers = 0;
	if (cJSON_IsNumber(num_samplers_json))
	{
		num_samplers = cJSON_GetNumberValue(num_samplers_json);
	}

	cJSON* num_storage_textures_json = cJSON_GetObjectItemCaseSensitive(json, "storage_textures");
	Uint32 num_storage_textures = 0;
	if (cJSON_IsNumber(num_storage_textures_json))
	{
		num_storage_textures = cJSON_GetNumberValue(num_storage_textures_json);
	}

	cJSON* num_storage_buffers_json = cJSON_GetObjectItemCaseSensitive(json, "storage_buffers");
	Uint32 num_storage_buffers = 0;
	if (cJSON_IsNumber(num_storage_buffers_json))
	{
		num_storage_buffers = cJSON_GetNumberValue(num_storage_buffers_json);
	}

	cJSON* num_uniform_buffers_json = cJSON_GetObjectItemCaseSensitive(json, "uniform_buffers");
	Uint32 num_uniform_buffers = 0;
	if (cJSON_IsNumber(num_uniform_buffers_json))
	{
		num_uniform_buffers = cJSON_GetNumberValue(num_uniform_buffers_json);
	}

	cJSON_Delete(json);
	SDL_free(json_raw);

	SDL_GPUShaderCreateInfo shaderInfo = 
    {
		.code = (const Uint8*) code,
		.code_size = code_size,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = num_samplers,
		.num_storage_textures = num_storage_textures,
		.num_storage_buffers = num_storage_buffers,
		.num_uniform_buffers = num_uniform_buffers
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(gpu_device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}