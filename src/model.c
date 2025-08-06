#include "model.h"
#include "globals.h"
#include "../external/cgltf.h"
#include "texture.h"

// TODO: remove other libc references from cgltf and replace with SDL versions
#define CGLTF_IMPLEMENTATION
#define CGLTF_MALLOC SDL_malloc
#define CGLTF_FREE SDL_free
#define CGLTF_ATOI SDL_atoi
#define CGLTF_ATOF SDL_atof
#define CGLTF_ATOLL(str) SDL_strtoll(str, NULL, 10)
#include "../external/cgltf.h"

bool Model_LoadAllModels(void)
{
    Array_Initialize(&models_unanimated);
    if (!models_unanimated.arr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize models array");
        return false;
    }

    Array_Initialize(&models_bone_animated);
    if (!models_bone_animated.arr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize models_bone_animated array");
        return false;
    }
    
    char path[512];
    SDL_snprintf(path, sizeof(path), "%smodels/%s", base_path, "_models_list.txt");
    size_t models_list_txt_size = 0;
    char* models_list_txt = (char*)SDL_LoadFile(path, &models_list_txt_size);
    if (!models_list_txt)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load glTF model list: %s", SDL_GetError());
        return false;
    }

    char* saveptr = NULL;
    char* line = SDL_strtok_r(models_list_txt, "\r\n", &saveptr);

    while (line != NULL)
    {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') 
            line++;
        
        // Skip empty lines
        if (*line != '\0')
        {
            if (!Model_Load(line))
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load model: %s", line);
                SDL_free(models_list_txt);
                return false;
            }
        }
        
        line = SDL_strtok_r(NULL, "\r\n", &saveptr);
    }
    
    SDL_free(models_list_txt);
    return true;
}

static bool Load_Unanimated(cgltf_data* gltf_data, cgltf_node* node, Model* model)
{
    if (!gltf_data || !node)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "NULL glTF data or node for unanimated model.");
        return false;
    }

    cgltf_mesh* mesh = node->mesh;
    if (!mesh || mesh->primitives_count == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Node has no mesh or primitives.");
        return false;
    }

    if (mesh->primitives_count > 1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Rendering models with multiple primitives is not supported. Found %zu primitives in node %s.", mesh->primitives_count, node->name);
        return false;
    }

    cgltf_primitive* primitive = mesh->primitives;

    if (primitive->type != cgltf_primitive_type_triangles)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: gltf primitive has type %d, expected type %d (triangles).", primitive->type, cgltf_primitive_type_triangles);
        return false;
    }

    cgltf_accessor* position_accessor = NULL;
    cgltf_accessor* texcoord_accessor = NULL;

    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute* attr = &(primitive->attributes[i]);
        if (attr->type == cgltf_attribute_type_position)
        {
            position_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0)
        {
            texcoord_accessor = attr->data;
        }
    }

    if (!position_accessor || !texcoord_accessor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing POSITION or TEXCOORD attributes.");
        return false;
    }

    Uint32 vertex_data_size = (Uint32)(sizeof(Vertex_PositionTexture) * position_accessor->count);
    model->vertex_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_data_size
        }
    );
    if (model->vertex_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create vertex buffer: %s", SDL_GetError());
        return false;
    }

    cgltf_accessor* index_accessor = primitive->indices;
    if (index_accessor == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing indices.");
        return false;
    }
    if (index_accessor->component_type != cgltf_component_type_r_16u)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Expected index component type to be Uint16, got %d", index_accessor->component_type);
        return false;
    }

    model->index_count = index_accessor->count;

    Uint32 index_data_size = (Uint32)(sizeof(Uint16) * model->index_count); // Assumes Uint16
    model->index_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_data_size
        }
    );
    if (model->index_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create index buffer: %s", SDL_GetError());
        return false;
    }

    Uint32 combined_data_size = vertex_data_size + index_data_size;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = combined_data_size
        }
    );
    if (transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    Uint8* transfer_buffer_mapped = SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);
    if (transfer_buffer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    for (size_t i = 0; i < position_accessor->count; ++i)
    {
        // Read position (vec3 float)
        if (!cgltf_accessor_read_float(position_accessor, i, (float*)(transfer_buffer_mapped + i * sizeof(Vertex_PositionTexture)), 3))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read position for vertex %zu", i);
            return false;
        }

        // Read texcoord (vec2 float), handle missing accessor
        if (!cgltf_accessor_read_float(texcoord_accessor, i, (float*)(transfer_buffer_mapped + i * sizeof(Vertex_PositionTexture) + sizeof(Vertex_Position)), 2))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read texcoord for vertex %zu", i);
            return false;
        }
    }

    size_t unpacked_count = cgltf_accessor_unpack_indices(index_accessor, (transfer_buffer_mapped + vertex_data_size), sizeof(Uint16), model->index_count);
    if (unpacked_count != model->index_count)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error unpacking gltf primitive indices: unexpected index_count (unpacked %zu, expected %u).", unpacked_count, model->index_count);
        return false;
    }

    SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

    char* texture_uri = NULL;
    if (primitive->material && primitive->material->has_pbr_metallic_roughness)
    {
        cgltf_texture* base_color_texture = primitive->material->pbr_metallic_roughness.base_color_texture.texture;
        if (base_color_texture && base_color_texture->image && base_color_texture->image->uri)
        {
            texture_uri = base_color_texture->image->uri;
        }
    }
    SDL_Surface* texture_surface = LoadImage(texture_uri);
    if (!texture_surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture from URI: %s", texture_uri);
        return false;
    }
    model->texture = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, // Assuming ABGR8888 surface
        .width = (Uint32)texture_surface->w,
        .height = (Uint32)texture_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET // COLOR_TARGET is needed for mipmap generation
    });
    if (model->texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create main texture: %s", SDL_GetError());
        return false;
    }
    Uint32 texture_data_size = (Uint32)texture_surface->w * texture_surface->h * 4;
    SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = texture_data_size
        }
    );
    if (texture_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create texture transfer buffer: %s", SDL_GetError());
        return false;
    }

    Uint8* texture_transfer_mapped = SDL_MapGPUTransferBuffer(gpu_device, texture_transfer_buffer, false);
    if (texture_transfer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map texture transfer buffer: %s", SDL_GetError());
        return false;
    }

    SDL_memcpy(texture_transfer_mapped, texture_surface->pixels, texture_data_size);
    SDL_UnmapGPUTransferBuffer(gpu_device, texture_transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (upload_command_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire upload command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        { 
            .transfer_buffer = transfer_buffer, 
            .offset = 0 
        },
        &(SDL_GPUBufferRegion)
        { 
            .buffer = model->vertex_buffer, 
            .offset = 0, 
            .size = vertex_data_size 
        },
        false
    );

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        {
            .transfer_buffer = transfer_buffer,
            .offset = vertex_data_size // Offset after vertex data
        },
        &(SDL_GPUBufferRegion)
        {
            .buffer = model->index_buffer,
            .offset = 0,
            .size = index_data_size
        },
        false
    );
    
    SDL_UploadToGPUTexture
    (
        copy_pass,
        &(SDL_GPUTextureTransferInfo)
        {
            .transfer_buffer = texture_transfer_buffer,
            .offset = 0,
            .pixels_per_row = (Uint32)texture_surface->w,
            .rows_per_layer = (Uint32)texture_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = model->texture,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_surface->w,
            .h = (Uint32)texture_surface->h,
            .d = 1
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, model->texture);

    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    // Cleanup Upload Resources
    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
    SDL_DestroySurface(texture_surface);

    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded unanimated model: %s", node->name);

    return true;
}

static bool Load_BoneAnimated(cgltf_data* gltf_data, cgltf_node* node, Model_BoneAnimated* model)
{
    cgltf_mesh* mesh;
    cgltf_skin* skin = node->skin;
    
    /*
        If this is a mixamo model...

        This armature node should have two children:
        * A node with the skin and mesh
        * The hip joint node (which is the root of the skeleton)
         
        This node will have its own rotation (up-direction fix) and scale.
        This corrective matrix is applied at the start of the joint matrix calculations
    */

    mat4 t_matrix, r_matrix, s_matrix;
    
    if (node->has_translation)
    {
        glm_translate_make(t_matrix, node->translation);
    }
    else
    {
        glm_mat4_identity(t_matrix);
    }
    if (node->has_rotation)
    {
        glm_quat_mat4(node->rotation, r_matrix);
    }
    else
    {
        glm_mat4_identity(r_matrix);
    }
    if (node->has_scale)
    {
        glm_scale_make(s_matrix, node->scale);
    }
    else
    {
        glm_mat4_identity(s_matrix);
    }
    glm_mat4_mul(t_matrix, r_matrix, model->armature_correction_matrix);
    glm_mat4_mul(model->armature_correction_matrix, s_matrix, model->armature_correction_matrix);

    glm_mat4_identity(model->model_matrix);
    
    if (skin)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Unexpected skin in armature root node: %s\nskin was expected to be in a child of this node", node->name);
        return false;
    }
    
    for (size_t i = 0; i < node->children_count; ++i)
    {
        skin = node->children[i]->skin;
        
        if (skin)
        {
            node = node->children[i];
            break;
        }
    }

    if (!skin)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Failed to load skin; node: %s", node->name);
        return false;
    }
        
    mesh = node->mesh;
    
    model->num_joints = (Uint8)node->skin->joints_count;
    model->joints = (Joint*)SDL_malloc(sizeof(Joint) * model->num_joints);
    if (model->joints == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for joints for skin: %s", node->name);
        return false;
    }

    /*
        I want to reference joints by their index in the joint matrix array, but...
        - gltf refers to them by their index in the gltf node array
        - cgltf references them via `cgltf_node*`, not an index

        Hence the conversion via `cgltf_node_index` and then my own `gltf_index_to_joint_mat_index`

        This is a constant time lookup, and if the bone animated model is the only thing in the gltf file, 
        the number of joints ~= the total number of nodes in the gltf file (it's ~ a perfect hash table)
    */
    Uint8 gltf_index_to_joint_mat_index[gltf_data->nodes_count];

    #define gltf_joint_node skin->joints[i]

    for (size_t i = 0; i < model->num_joints; i++)
    {
        gltf_index_to_joint_mat_index[(Uint8)cgltf_node_index(gltf_data, gltf_joint_node)] = i;
    }

    for (size_t i = 0; i < model->num_joints; i++)
    {
        model->joints[i].num_children = gltf_joint_node->children_count;

        for (size_t ii = 0; ii < gltf_joint_node->children_count && ii < MAX_CHILDREN_PER_JOINT; ii++)
        {
            model->joints[i].children[ii] = gltf_index_to_joint_mat_index[(Uint8)cgltf_node_index(gltf_data, gltf_joint_node->children[ii])];
        }

        // these vectors should put the skeleton in the default A/T Pose
        
        if (gltf_joint_node->has_translation)
        {
            glm_vec3_copy(gltf_joint_node->translation, model->joints[i].translation);
        }
        else
        {
            glm_vec3_zero(model->joints[i].translation);
        }
        if (gltf_joint_node->has_rotation)
        {
            glm_quat_copy(gltf_joint_node->rotation, model->joints[i].rotation);
        }
        else
        {
            glm_quat_identity(model->joints[i].rotation);
        }
        if (gltf_joint_node->has_scale)
        {
            glm_vec3_copy(gltf_joint_node->scale, model->joints[i].scale);
        }
        else
        {
            glm_vec3_one(model->joints[i].scale);
        }
    }

    #undef gltf_joint_node

    mat4 inverse_bind_matrices[model->num_joints];

    cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, (float*)inverse_bind_matrices, 16 * model->num_joints); 
    
    // populate the joints with inverse bind matrices
    for (size_t i = 0; i < model->num_joints; ++i)
    {
        SDL_memcpy(model->joints[i].inverse_bind_matrix, inverse_bind_matrices[i], sizeof(mat4));        
    }

    // load animations

    model->num_skeletal_animations = (Uint8)gltf_data->animations_count;
    model->skeletal_animations = (Animation_Skeletal*)SDL_malloc(sizeof(Animation_Skeletal) * model->num_skeletal_animations);
    if (model->skeletal_animations == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for skeletal animations for skin: %s", node->name);
        return false;
    }

    for (size_t i = 0; i < model->num_skeletal_animations; ++i)
    {
        model->skeletal_animations[i].animation_id = (Animation_Skeletal_ID)SDL_atoi(gltf_data->animations[i].name);
        model->skeletal_animations[i].num_key_frames = (Uint16)gltf_data->animations[i].samplers[0].input->count;
        model->skeletal_animations[i].num_joint_updates_per_frame = (Uint16)gltf_data->animations[i].channels_count;
        model->skeletal_animations[i].key_frame_times = (float*)SDL_malloc(sizeof(float) * model->skeletal_animations[i].num_key_frames);
        model->skeletal_animations[i].joint_updates = (Joint_Update*)SDL_malloc(sizeof(Joint_Update) * model->skeletal_animations[i].num_joint_updates_per_frame * model->skeletal_animations[i].num_key_frames);
        if (model->skeletal_animations[i].key_frame_times == NULL || model->skeletal_animations[i].joint_updates == NULL)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for key frame times or joint updates for skeletal animation %zu in skin: %s", i, node->name);
            SDL_free(model->skeletal_animations[i].key_frame_times);
            SDL_free(model->skeletal_animations[i].joint_updates);
            return false;
        }
        cgltf_animation* animation = &gltf_data->animations[i];

        // Copy key frame times
        // we assume each channel uses the same input accessor for the key frame times
        cgltf_accessor* input_accessor = animation->samplers[0].input;
        cgltf_accessor_unpack_floats(input_accessor, model->skeletal_animations[i].key_frame_times, model->skeletal_animations[i].num_key_frames);

        // Copy joint updates
        // gltf is structured such that each channel tracks how a single joint is updated over time
        // I want to invert this so that each key frame has all joint updates for that frame adjacent in memory
        for (size_t j = 0; j < model->skeletal_animations[i].num_key_frames; ++j)
        {
            for (size_t k = 0; k < model->skeletal_animations[i].num_joint_updates_per_frame; ++k)
            {
                Joint_Update* joint_update = &model->skeletal_animations[i].joint_updates[j * model->skeletal_animations[i].num_joint_updates_per_frame + k];
                cgltf_animation_channel* channel = &animation->channels[k];
                cgltf_accessor* output_accessor = channel->sampler->output;
                joint_update->joint_index = gltf_index_to_joint_mat_index[(Uint8)cgltf_node_index(gltf_data, channel->target_node)];
        
                // Read joint update data
                switch (channel->target_path)
                {
                    case cgltf_animation_path_type_translation:
                        joint_update->joint_update_type = JOINT_UPDATE_TYPE_TRANSLATION;
                        cgltf_accessor_read_float(output_accessor, j, joint_update->translation, 3);
                        break;
                    case cgltf_animation_path_type_rotation:
                        joint_update->joint_update_type = JOINT_UPDATE_TYPE_ROTATION;
                        cgltf_accessor_read_float(output_accessor, j, joint_update->rotation, 4);
                        break;
                    case cgltf_animation_path_type_scale:
                        joint_update->joint_update_type = JOINT_UPDATE_TYPE_SCALE;
                        cgltf_accessor_read_float(output_accessor, j, joint_update->scale, 3);
                        break;
                    default:
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported animation path type: %d", channel->target_path);
                        return false;
                }
            }
        }
    }

    // load vertex attributes

    if (mesh->primitives_count != 1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Expected mesh with one primitive. Found %zu primitives in node: %s.", mesh->primitives_count, node->name);
        return false;
    }

    cgltf_primitive* primitive = mesh->primitives;

    if (primitive->type != cgltf_primitive_type_triangles)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: gltf primitive has type %d, expected type %d (triangles).", primitive->type, cgltf_primitive_type_triangles);
        return false;
    }

    cgltf_accessor* position_accessor = NULL;
    cgltf_accessor* texcoord_accessor = NULL;
    cgltf_accessor* joint_ids_accessor = NULL;
    cgltf_accessor* joint_weights_accessor = NULL;
    
    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute* attr = &(primitive->attributes[i]);
        if (attr->type == cgltf_attribute_type_position)
        {
            position_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0)
        {
            texcoord_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_joints && attr->index == 0)
        {
            joint_ids_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_weights && attr->index == 0)
        {
            joint_weights_accessor = attr->data;
        }
    }
    if (!position_accessor || !texcoord_accessor || !joint_ids_accessor || !joint_weights_accessor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing POSITION, TEXCOORD, JOINTS or WEIGHTS attributes.");
        return false;
    }
    if (joint_ids_accessor->component_type != cgltf_component_type_r_8u || joint_weights_accessor->component_type != cgltf_component_type_r_32f)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Expected JOINTS to be Uint8 and WEIGHTS to be Float32.");
        return false;
    }

    Uint32 vertex_data_size = (Uint32)(sizeof(Vertex_BoneAnimated) * position_accessor->count);
    model->vertex_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_data_size
        }
    );
    if (model->vertex_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create vertex buffer: %s", SDL_GetError());
        return false;
    }

    cgltf_accessor* index_accessor = primitive->indices;
    if (index_accessor == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing indices.");
        return false;
    }
    if (index_accessor->component_type != cgltf_component_type_r_16u)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Expected index component type to be Uint16, got %d", index_accessor->component_type);
        return false;
    }

    model->index_count = index_accessor->count;

    Uint32 index_data_size = (Uint32)(sizeof(Uint16) * model->index_count); // Assumes Uint16
    model->index_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_data_size
        }
    );
    if (model->index_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create index buffer: %s", SDL_GetError());
        return false;
    }

    Uint32 combined_data_size = vertex_data_size + index_data_size;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = combined_data_size
        }
    );
    if (transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    Uint8* transfer_buffer_mapped = SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);
    if (transfer_buffer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }
    Vertex_BoneAnimated* vertices_mapped = (Vertex_BoneAnimated*)transfer_buffer_mapped;

    // using buffers directly myself because I was having problems with the cgltf_accessor_read_ functions
    const uint8_t* pos_data_base = cgltf_buffer_view_data(position_accessor->buffer_view);
    if (pos_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get position data buffer view.");
        return false;
    }
    pos_data_base += position_accessor->offset;

    const uint8_t* texcoord_data_base = cgltf_buffer_view_data(texcoord_accessor->buffer_view);
    if (texcoord_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get texcoord data buffer view.");
        return false; 
    }
    texcoord_data_base += texcoord_accessor->offset;

    const uint8_t* joint_ids_data_base = cgltf_buffer_view_data(joint_ids_accessor->buffer_view);
    if (joint_ids_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get joint IDs data buffer view.");
        return false; 
    }
    joint_ids_data_base += joint_ids_accessor->offset;

    const uint8_t* joint_weights_data_base = cgltf_buffer_view_data(joint_weights_accessor->buffer_view);
    if (joint_weights_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get joint weights data buffer view.");
        return false; 
    }
    joint_weights_data_base += joint_weights_accessor->offset;

    for (size_t i = 0; i < position_accessor->count; i++)
    {
        Vertex_BoneAnimated* dest_vertex = &vertices_mapped[i];

        const void* src_pos = pos_data_base + i * position_accessor->stride;
        memcpy(&dest_vertex->x, src_pos, sizeof(float) * 3);

        const void* src_texcoord = texcoord_data_base + i * texcoord_accessor->stride;
        memcpy(&dest_vertex->u, src_texcoord, sizeof(float) * 2);

        const void* src_joint_ids = joint_ids_data_base + i * joint_ids_accessor->stride;
        memcpy(dest_vertex->joint_ids, src_joint_ids, sizeof(uint8_t) * MAX_JOINTS_PER_VERTEX);

        const void* src_weights = joint_weights_data_base + i * joint_weights_accessor->stride;
        memcpy(dest_vertex->weights, src_weights, sizeof(float) * MAX_JOINTS_PER_VERTEX);
    }

    size_t unpacked_count = cgltf_accessor_unpack_indices(index_accessor, (transfer_buffer_mapped + vertex_data_size), sizeof(Uint16), model->index_count);
    if (unpacked_count != model->index_count)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error unpacking gltf primitive indices: unexpected index_count (unpacked %zu, expected %u).", unpacked_count, model->index_count);
        return false;
    }

    SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

    char* texture_uri = NULL;
    if (primitive->material && primitive->material->has_pbr_metallic_roughness)
    {
        cgltf_texture* base_color_texture = primitive->material->pbr_metallic_roughness.base_color_texture.texture;
        if (base_color_texture && base_color_texture->image && base_color_texture->image->uri)
        {
            texture_uri = base_color_texture->image->uri;
        }
    }
    SDL_Surface* texture_surface = LoadImage(texture_uri);
    if (!texture_surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture from URI: %s", texture_uri);
        return false;
    }
    model->texture = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, // Assuming ABGR8888 surface
        .width = (Uint32)texture_surface->w,
        .height = (Uint32)texture_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET // COLOR_TARGET is needed for mipmap generation
    });
    if (model->texture == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create main texture: %s", SDL_GetError());
        return false;
    }
    Uint32 texture_data_size = (Uint32)texture_surface->w * texture_surface->h * 4;
    SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer
    (
        gpu_device,
        &(SDL_GPUTransferBufferCreateInfo)
        {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = texture_data_size
        }
    );
    if (texture_transfer_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create texture transfer buffer: %s", SDL_GetError());
        return false;
    }

    Uint8* texture_transfer_mapped = SDL_MapGPUTransferBuffer(gpu_device, texture_transfer_buffer, false);
    if (texture_transfer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map texture transfer buffer: %s", SDL_GetError());
        return false;
    }

    SDL_memcpy(texture_transfer_mapped, texture_surface->pixels, texture_data_size);
    SDL_UnmapGPUTransferBuffer(gpu_device, texture_transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (upload_command_buffer == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire upload command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        { 
            .transfer_buffer = transfer_buffer, 
            .offset = 0 
        },
        &(SDL_GPUBufferRegion)
        { 
            .buffer = model->vertex_buffer, 
            .offset = 0, 
            .size = vertex_data_size 
        },
        false
    );

    SDL_UploadToGPUBuffer
    (
        copy_pass,
        &(SDL_GPUTransferBufferLocation)
        {
            .transfer_buffer = transfer_buffer,
            .offset = vertex_data_size // Offset after vertex data
        },
        &(SDL_GPUBufferRegion)
        {
            .buffer = model->index_buffer,
            .offset = 0,
            .size = index_data_size
        },
        false
    );
    
    SDL_UploadToGPUTexture
    (
        copy_pass,
        &(SDL_GPUTextureTransferInfo)
        {
            .transfer_buffer = texture_transfer_buffer,
            .offset = 0,
            .pixels_per_row = (Uint32)texture_surface->w,
            .rows_per_layer = (Uint32)texture_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = model->texture,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_surface->w,
            .h = (Uint32)texture_surface->h,
            .d = 1
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, model->texture);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
    SDL_DestroySurface(texture_surface);

    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded bone animated model: %s", node->name);

    return true;
}

bool Model_Load(const char* filename)
{
    char model_path[512];
    SDL_snprintf(model_path, sizeof(model_path), "%smodels/%s", base_path, filename);
    size_t gltf_file_buffer_size = 0;
    void* gltf_file_buffer = SDL_LoadFile(model_path, &gltf_file_buffer_size);
    if (!gltf_file_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load glTF model file: %s", SDL_GetError());
        return false;
    }
    
    cgltf_options options = {0};
    cgltf_data* gltf_data = NULL;
    cgltf_result result = cgltf_parse(&options, gltf_file_buffer, gltf_file_buffer_size, &gltf_data);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "cgltf_parse_file failed: %d for %s", result, model_path);
        return false;
    }

    result = cgltf_load_buffers(&options, gltf_data, model_path);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "cgltf_load_buffers failed: %d", result);
        cgltf_free(gltf_data);
        return false;
    }

    result = cgltf_validate(gltf_data);
    if (result != cgltf_result_success)
    {
        // Continue even if validation fails? might still work
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "cgltf_validate failed: %d (continuing anyway)", result);
    }

	// assume we only have the single default scene
    #define root_nodes gltf_data->scene->nodes
    if (!gltf_data->scene || !root_nodes)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No root nodes found in the glTF scene.");
        cgltf_free(gltf_data);
        return false;
    }
	for (size_t i = 0; i < gltf_data->scene->nodes_count; i++)
	{
		if (root_nodes[i]->name == NULL)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Root Node %zu has no name; nodes MUST be named and prefixed with numerical Model_Type", i);
            cgltf_free(gltf_data);
            return false;
        }
        switch (SDL_atoi(root_nodes[i]->name))
        {
            case MODEL_TYPE_UNRENDERED:
                break;
            case MODEL_TYPE_UNANIMATED:
            {
                Model new_model = {0};
                if (!Load_Unanimated(gltf_data, root_nodes[i], &new_model))
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load unanimated model of node %s", root_nodes[i]->name);
                    Model_Free(&new_model);
                    cgltf_free(gltf_data);
                    return false;
                }
                Array_Append(&models_unanimated, new_model);
                break;
            }
            case MODEL_TYPE_BONE_ANIMATED:
            {
                Model_BoneAnimated model_bone_animated = {0};
                if (!Load_BoneAnimated(gltf_data, root_nodes[i], &model_bone_animated))
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load unanimated model of node %s", root_nodes[i]->name);
                    Model_BoneAnimated_Free(&model_bone_animated);
                    cgltf_free(gltf_data);
                    return false;
                }
                Array_Append(&models_bone_animated, model_bone_animated);
                break;
            }
            case MODEL_TYPE_RIGID_ANIMATED:
                break;
            case MODEL_TYPE_INSTANCED:
                break;
            default:
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Node %s has unsupported model type", root_nodes[i]->name);
                cgltf_free(gltf_data);
                return false;
        }
	}

    cgltf_free(gltf_data); // maps to SDL_free

    return true;
    #undef root_nodes
}

void Model_Free(Model* model)
{
    SDL_ReleaseGPUBuffer(gpu_device, model->vertex_buffer);
    SDL_ReleaseGPUBuffer(gpu_device, model->index_buffer);
    SDL_ReleaseGPUTexture(gpu_device, model->texture);
    SDL_memset(model, 0, sizeof(Model));
}

void Model_BoneAnimated_Free(Model_BoneAnimated* model)
{
    SDL_ReleaseGPUBuffer(gpu_device, model->vertex_buffer);
    SDL_ReleaseGPUBuffer(gpu_device, model->index_buffer);
    SDL_ReleaseGPUTexture(gpu_device, model->texture);
    SDL_free(model->joints);
    SDL_memset(model, 0, sizeof(Model_BoneAnimated));
}

static void calculate_joint_matrices(Joint* joint, mat4 parent_global_transform, Uint8* joint_matrices_out, Joint* root_joint) 
{
    mat4 local_transform;
    mat4 t_matrix, r_matrix, s_matrix;
    
    glm_translate_make(t_matrix, joint->translation);
    glm_quat_mat4(joint->rotation, r_matrix);
    glm_scale_make(s_matrix, joint->scale);
    
    glm_mat4_mul(t_matrix, r_matrix, local_transform);
    glm_mat4_mul(local_transform, s_matrix, local_transform);
    
    mat4 global_transform;
    glm_mat4_mul(parent_global_transform, local_transform, global_transform);
    
    // Calculate the index of the current joint
    size_t joint_index = joint - root_joint;
    
    // Get a pointer to the correct location in the output buffer
    mat4* destination_matrix = (mat4*)(joint_matrices_out + joint_index * sizeof(mat4));

    // Calculate the final skinning matrix and write it to the destination
    glm_mat4_mul(global_transform, joint->inverse_bind_matrix, *destination_matrix);
    
    for (int i = 0; i < joint->num_children; i++) 
    {
        calculate_joint_matrices(&root_joint[joint->children[i]], global_transform, joint_matrices_out, root_joint);
    }
}

bool Model_JointMat_UpdateAndUpload()
{
    SDL_GPUCommandBuffer* command_buffer_joint_matrix = SDL_AcquireGPUCommandBuffer(gpu_device);
    {
        void* mapped = SDL_MapGPUTransferBuffer(gpu_device, joint_matrix_transfer_buffer, true);
        if (!mapped) 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
            return false;
        }
        
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(command_buffer_joint_matrix);
        size_t current_offset_bytes = 0;

        for (size_t i = 0; i < models_bone_animated.len; i++)
        {
            // update animations

            Model_BoneAnimated* model = &models_bone_animated.arr[i];

            model->animation_progress += delta_time;
            
            Animation_Skeletal* animation = &model->skeletal_animations[model->active_animation_index];
            
            Uint16 frame_index = 0;
            bool animation_finished = true;

            for (size_t ii = 0; ii < animation->num_key_frames; ii++)
            {
                if (animation->key_frame_times[ii] > model->animation_progress)
                {
                    animation_finished = false;
                    break;
                }
                frame_index = (Uint16)ii;
            }

            if (animation_finished && animation->is_looping)
            {
                model->animation_progress = 0.0f;
                frame_index = 0;
            }
            else if (animation_finished)
            {
                // TODO this needs some kind of state machine to determine what animation to play next
                // for now, just reset to the first frame of the first animation
                model->active_animation_index = 0;
                animation = &model->skeletal_animations[model->active_animation_index];
                model->animation_progress = 0.0f;
                frame_index = 0;
            }
            
            float interpolant = (model->animation_progress - animation->key_frame_times[frame_index]) / 
                (animation->key_frame_times[frame_index + 1] - animation->key_frame_times[frame_index]);

            for (size_t ii = 0; ii < animation->num_joint_updates_per_frame; ii++)
            {
                Joint_Update* prev_joint_update = &animation->joint_updates[ii + frame_index * animation->num_joint_updates_per_frame];
                Joint_Update* next_joint_update = &animation->joint_updates[ii + (frame_index + 1) * animation->num_joint_updates_per_frame];
                switch (prev_joint_update->joint_update_type)
                {
                    case JOINT_UPDATE_TYPE_TRANSLATION:
                        glm_vec3_lerp(prev_joint_update->translation, next_joint_update->translation, interpolant, model->joints[prev_joint_update->joint_index].translation);
                        break;
                    case JOINT_UPDATE_TYPE_ROTATION:
                        glm_quat_slerp(prev_joint_update->rotation, next_joint_update->rotation, interpolant, model->joints[prev_joint_update->joint_index].rotation);
                        break;
                    case JOINT_UPDATE_TYPE_SCALE:
                        glm_vec3_lerp(prev_joint_update->scale, next_joint_update->scale, interpolant, model->joints[prev_joint_update->joint_index].scale);
                        break;
                    default:
                        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Unknown joint update type: %d", prev_joint_update->joint_update_type);
                }
            }

            // update joint matrices

            models_bone_animated.arr[i].storage_buffer_offset_bytes = current_offset_bytes;

            // for mixamo models, the first joint should also be the root node 
            // in the future this may need to be determined by checking the joint hierarchy
            
            calculate_joint_matrices
            (
                models_bone_animated.arr[i].joints, 
                models_bone_animated.arr[i].armature_correction_matrix, // parent transform
                (Uint8*)mapped + current_offset_bytes, models_bone_animated.arr[i].joints // root joint
            );

            current_offset_bytes += models_bone_animated.arr[i].num_joints * sizeof(mat4);
        }

        SDL_GPUTransferBufferLocation source = 
        {
            .transfer_buffer = joint_matrix_transfer_buffer,
            .offset = 0
        };
        
        SDL_GPUBufferRegion destination = 
        {
            .buffer = joint_matrix_storage_buffer,
            .offset = 0,
            .size = current_offset_bytes
        };
            
        SDL_UploadToGPUBuffer(copyPass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, joint_matrix_transfer_buffer);
        SDL_EndGPUCopyPass(copyPass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer_joint_matrix);

    return true;
}