#include "model.h"
#include "globals.h"
#include "texture.h"
#include "physics.h"

// TODO: remove other libc references from cgltf and replace with SDL versions
#define CGLTF_IMPLEMENTATION
#define CGLTF_MALLOC SDL_malloc
#define CGLTF_FREE SDL_free
#define CGLTF_ATOI SDL_atoi
#define CGLTF_ATOF SDL_atof
#define CGLTF_ATOLL(str) SDL_strtoll(str, NULL, 10)
#include "../external/cgltf.h"

// TODO upload everything in one copy pass instead of a dedicated copy pass per model
bool Model_Load_AllScenes(void)
{   
    char path[MAXIMUM_URI_LENGTH];
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
            if (!Model_Load_Scene(line))
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

bool Model_Load_Scene(const char* filename)
{
    char model_path[MAXIMUM_URI_LENGTH];
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "cgltf_parse_file failed: cgltf_result %d for %s", result, model_path);
        return false;
    }

    result = cgltf_load_buffers(&options, gltf_data, model_path);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "cgltf_load_buffers failed: cgltf_result %d", result);
        cgltf_free(gltf_data);
        return false;
    }

    result = cgltf_validate(gltf_data);
    if (result != cgltf_result_success)
    {
        // Continue even if validation fails? might still work
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "cgltf_validate failed: cgltf_result %d (continuing anyway)", result);
    }

	// assume we only have the single default scene
    #define root_nodes gltf_data->scene->nodes
    if (!gltf_data->scene || !root_nodes)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No nodes found in glTF scene.");
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
        if (!Model_Load(gltf_data, root_nodes[i]))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load model from node %zu: %s", i, root_nodes[i]->name);
            cgltf_free(gltf_data);
            return false;
        }
	}

    cgltf_free(gltf_data); // maps to SDL_free

    return true;
    #undef root_nodes
}

bool Model_Load(cgltf_data* gltf_data, cgltf_node* node)
{
    Model_Type model_type = (Model_Type)SDL_atoi(node->name);
    if (model_type == MODEL_TYPE_UNKNOWN)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Node %s has unknown model type", node->name);
        return false;
    }

    if (model_type == MODEL_TYPE_COLLIDER)
    {
        if (!Model_Load_Collider(gltf_data, node))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load collider model from node: %s", node->name);
            return false;
        }
        else return true;
    }

    /**************** Animation / Rigging ****************/

    Animation_Rig animation_rig = {0};
    if (model_type == MODEL_TYPE_BONE_ANIMATED_MIXAMO)
    {
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
        
        glm_mat4_mul(t_matrix, r_matrix, animation_rig.armature_correction_matrix);
        glm_mat4_mul(animation_rig.armature_correction_matrix, s_matrix, animation_rig.armature_correction_matrix);

        cgltf_skin* skin = node->skin;
        
        if (skin)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Unexpected skin in armature root node: %s\nskin was expected to be in a child of this node", node->name);
            return false;
        }
        
        // we don't assume the order of the children,
        // we just know one of them should have the skin
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

        animation_rig.num_joints = (Uint8)node->skin->joints_count;
        animation_rig.joints = (Joint*)SDL_malloc(sizeof(Joint) * animation_rig.num_joints);
        if (animation_rig.joints == NULL)
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

        for (size_t i = 0; i < animation_rig.num_joints; i++)
        {
            gltf_index_to_joint_mat_index[(Uint8)cgltf_node_index(gltf_data, gltf_joint_node)] = i;
        }

        for (size_t i = 0; i < animation_rig.num_joints; i++)
        {
            animation_rig.joints[i].num_children = gltf_joint_node->children_count;

            for (size_t ii = 0; ii < gltf_joint_node->children_count && ii < MAX_CHILDREN_PER_JOINT; ii++)
            {
                animation_rig.joints[i].children[ii] = gltf_index_to_joint_mat_index[(Uint8)cgltf_node_index(gltf_data, gltf_joint_node->children[ii])];
            }

            // these vectors should put the skeleton in the default A/T Pose
            
            if (gltf_joint_node->has_translation)
            {
                glm_vec3_copy(gltf_joint_node->translation, animation_rig.joints[i].translation);
            }
            else
            {
                glm_vec3_zero(animation_rig.joints[i].translation);
            }
            if (gltf_joint_node->has_rotation)
            {
                glm_quat_copy(gltf_joint_node->rotation, animation_rig.joints[i].rotation);
            }
            else
            {
                glm_quat_identity(animation_rig.joints[i].rotation);
            }
            if (gltf_joint_node->has_scale)
            {
                glm_vec3_copy(gltf_joint_node->scale, animation_rig.joints[i].scale);
            }
            else
            {
                glm_vec3_one(animation_rig.joints[i].scale);
            }
        }

        #undef gltf_joint_node

        mat4 inverse_bind_matrices[animation_rig.num_joints];

        cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, (float*)inverse_bind_matrices, 16 * animation_rig.num_joints); 
        
        // populate the joints with inverse bind matrices
        for (size_t i = 0; i < animation_rig.num_joints; ++i)
        {
            SDL_memcpy(animation_rig.joints[i].inverse_bind_matrix, inverse_bind_matrices[i], sizeof(mat4));        
        }

        // load animations

        animation_rig.num_skeletal_animations = (Uint8)gltf_data->animations_count;
        animation_rig.skeletal_animations = (Animation_Skeletal*)SDL_malloc(sizeof(Animation_Skeletal) * animation_rig.num_skeletal_animations);
        if (animation_rig.skeletal_animations == NULL)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for skeletal animations for skin: %s", node->name);
            return false;
        }

        for (size_t i = 0; i < animation_rig.num_skeletal_animations; ++i)
        {
            animation_rig.skeletal_animations[i].animation_id = (Animation_Skeletal_ID)SDL_atoi(gltf_data->animations[i].name);
            animation_rig.skeletal_animations[i].num_key_frames = (Uint16)gltf_data->animations[i].samplers[0].input->count;
            animation_rig.skeletal_animations[i].num_joint_updates_per_frame = (Uint16)gltf_data->animations[i].channels_count;
            animation_rig.skeletal_animations[i].key_frame_times = (float*)SDL_malloc(sizeof(float) * animation_rig.skeletal_animations[i].num_key_frames);
            animation_rig.skeletal_animations[i].joint_updates = (Joint_Update*)SDL_malloc(sizeof(Joint_Update) * animation_rig.skeletal_animations[i].num_joint_updates_per_frame * animation_rig.skeletal_animations[i].num_key_frames);
            if (animation_rig.skeletal_animations[i].key_frame_times == NULL || animation_rig.skeletal_animations[i].joint_updates == NULL)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for key frame times or joint updates for skeletal animation %zu in skin: %s", i, node->name);
                SDL_free(animation_rig.skeletal_animations[i].key_frame_times);
                SDL_free(animation_rig.skeletal_animations[i].joint_updates);
                return false;
            }
            cgltf_animation* animation = &gltf_data->animations[i];

            // Copy key frame times
            // we assume each channel uses the same input accessor for the key frame times
            cgltf_accessor* input_accessor = animation->samplers[0].input;
            cgltf_accessor_unpack_floats(input_accessor, animation_rig.skeletal_animations[i].key_frame_times, animation_rig.skeletal_animations[i].num_key_frames);

            // Copy joint updates
            // gltf is structured such that each channel tracks how a single joint is updated over time
            // I want to invert this so that each key frame has all joint updates for that frame adjacent in memory
            for (size_t j = 0; j < animation_rig.skeletal_animations[i].num_key_frames; ++j)
            {
                for (size_t k = 0; k < animation_rig.skeletal_animations[i].num_joint_updates_per_frame; ++k)
                {
                    Joint_Update* joint_update = &animation_rig.skeletal_animations[i].joint_updates[j * animation_rig.skeletal_animations[i].num_joint_updates_per_frame + k];
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
    }

    /**************** Mesh ****************/

    if (node->mesh->primitives_count != 1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Expected mesh with one primitive. Found %zu primitives in node: %s.", node->mesh->primitives_count, node->name);
        return false;
    }

    cgltf_primitive* primitive = node->mesh->primitives;
    if (primitive->type != cgltf_primitive_type_triangles)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: gltf primitive has type %d, expected type %d (triangles).", primitive->type, cgltf_primitive_type_triangles);
        return false;
    }

    cgltf_accessor* position_accessor = NULL;
    cgltf_accessor* normal_accessor = NULL;
    cgltf_accessor* texcoord_accessor = NULL;
    cgltf_accessor* tangent_accessor = NULL;
    cgltf_accessor* joint_ids_accessor = NULL;
    cgltf_accessor* joint_weights_accessor = NULL;
    
    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute* attr = &(primitive->attributes[i]);
        if (attr->type == cgltf_attribute_type_position)
        {
            position_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_normal)
        {
            normal_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0)
        {
            texcoord_accessor = attr->data;
        }
        else if (attr->type == cgltf_attribute_type_tangent)
        {
            tangent_accessor = attr->data;
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

    if (!position_accessor || !normal_accessor || !texcoord_accessor || !tangent_accessor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing at least one attribute: POSITION, NORMAL, TEXCOORD, TANGENT.");
        return false;
    }

    if (position_accessor->component_type != cgltf_component_type_r_32f ||
        normal_accessor->component_type   != cgltf_component_type_r_32f ||
        texcoord_accessor->component_type != cgltf_component_type_r_32f ||
        tangent_accessor->component_type != cgltf_component_type_r_32f) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Expected POSITION, NORMAL, TEXCOORD, TANGENT to be Float32. Current gltf loader does not support automatic conversion.");
        return false;
    }

    if (model_type == MODEL_TYPE_BONE_ANIMATED_MIXAMO || model_type == MODEL_TYPE_BONE_ANIMATED)
    {    
        if (!joint_ids_accessor || !joint_weights_accessor)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing at least one attribute: JOINTS, WEIGHTS.");
            return false;
        }
        if (joint_ids_accessor->component_type != cgltf_component_type_r_8u || joint_weights_accessor->component_type != cgltf_component_type_r_32f)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Expected JOINTS to be Uint8 and WEIGHTS to be Float32.");
            return false;
        }
    }

    Uint32 vertex_data_size;
    if (model_type == MODEL_TYPE_BONE_ANIMATED_MIXAMO || model_type == MODEL_TYPE_BONE_ANIMATED)
        vertex_data_size = (Uint32)(sizeof(Vertex_BoneAnimated) * position_accessor->count);
    else
        vertex_data_size = (Uint32)(sizeof(Vertex_PBR) * position_accessor->count);

    Mesh mesh = {0};
    mesh.vertex_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_data_size
        }
    );
    if (mesh.vertex_buffer == NULL)
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

    mesh.index_count = index_accessor->count;

    Uint32 index_data_size = (Uint32)(sizeof(Uint16) * mesh.index_count);
    mesh.index_buffer = SDL_CreateGPUBuffer
    (
        gpu_device,
        &(SDL_GPUBufferCreateInfo)
        {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_data_size
        }
    );
    if (mesh.index_buffer == NULL)
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

    uint8_t* transfer_buffer_mapped = SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);
    if (transfer_buffer_mapped == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map vertex/index transfer buffer: %s", SDL_GetError());
        return false;
    }

    // using buffers directly myself because there is a bug which seems to stem from cgltf_accessor_read_uint,
    // which is supposed to read (32 bit) unsigned ints. The joint IDs are stored as 8 bit uints
    // You would think since there are conveniently 4 joint IDs per vertex, that reading them 
    // as a single 32 bit uint would work, and yet for some reason it is broken.
    // that approach is comment out below

    const uint8_t* pos_data_base = cgltf_buffer_view_data(position_accessor->buffer_view);
    if (pos_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get position data buffer view.");
        return false;
    }
    pos_data_base += position_accessor->offset;

    const uint8_t* normal_data_base = cgltf_buffer_view_data(normal_accessor->buffer_view);
    if (normal_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get normal data buffer view.");
        return false; 
    }
    normal_data_base += normal_accessor->offset;

    const uint8_t* texcoord_data_base = cgltf_buffer_view_data(texcoord_accessor->buffer_view);
    if (texcoord_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get texcoord data buffer view.");
        return false; 
    }
    texcoord_data_base += texcoord_accessor->offset;

    const uint8_t* tangent_data_base = cgltf_buffer_view_data(tangent_accessor->buffer_view);
    if (tangent_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get tangent data buffer view.");
        return false; 
    }
    tangent_data_base += tangent_accessor->offset;

    if (model_type == MODEL_TYPE_BONE_ANIMATED_MIXAMO || model_type == MODEL_TYPE_BONE_ANIMATED)
    {
        const uint8_t* joint_ids_data_base= cgltf_buffer_view_data(joint_ids_accessor->buffer_view);
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
            Vertex_BoneAnimated* dest_vertex = &((Vertex_BoneAnimated*)transfer_buffer_mapped)[i];

            const void* src_pos = pos_data_base + i * position_accessor->stride;
            memcpy(&dest_vertex->x, src_pos, sizeof(float) * 3);

            const void* src_normal = normal_data_base + i * normal_accessor->stride;
            memcpy(&dest_vertex->nx, src_normal, sizeof(float) * 3);

            const void* src_texcoord = texcoord_data_base + i * texcoord_accessor->stride;
            memcpy(&dest_vertex->u, src_texcoord, sizeof(float) * 2);

            const void* src_tangent = tangent_data_base + i * tangent_accessor->stride;
            memcpy(&dest_vertex->tx, src_tangent, sizeof(float) * 4);

            const void* src_joint_ids = joint_ids_data_base + i * joint_ids_accessor->stride;
            memcpy(dest_vertex->joint_ids, src_joint_ids, sizeof(uint8_t) * MAX_JOINTS_PER_VERTEX);

            const void* src_weights = joint_weights_data_base + i * joint_weights_accessor->stride;
            memcpy(dest_vertex->weights, src_weights, sizeof(float) * MAX_JOINTS_PER_VERTEX);
        }
    }
    else
    {
        for (size_t i = 0; i < position_accessor->count; i++)
        {
            Vertex_PBR* dest_vertex = &((Vertex_PBR*)transfer_buffer_mapped)[i];

            const void* src_pos = pos_data_base + i * position_accessor->stride;
            memcpy(&dest_vertex->x, src_pos, sizeof(float) * 3);

            const void* src_normal = normal_data_base + i * normal_accessor->stride;
            memcpy(&dest_vertex->nx, src_normal, sizeof(float) * 3);

            const void* src_texcoord = texcoord_data_base + i * texcoord_accessor->stride;
            memcpy(&dest_vertex->u, src_texcoord, sizeof(float) * 2);

            const void* src_tangent = tangent_data_base + i * tangent_accessor->stride;
            memcpy(&dest_vertex->tx, src_tangent, sizeof(float) * 4);
        }
    }
    // for (size_t i = 0; i < position_accessor->count; ++i)
    // {
    //     if (!cgltf_accessor_read_float(position_accessor, i, &transfer_buffer_mapped[i].x, 3))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read position for vertex %zu", i);
    //         return false;
    //     }
    //     if (!cgltf_accessor_read_float(normal_accessor, i, &transfer_buffer_mapped[i].nx, 3))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read normal for vertex %zu", i);
    //         return false;
    //     }
    //     if (!cgltf_accessor_read_float(texcoord_accessor, i, &transfer_buffer_mapped[i].u, 2))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read texcoord for vertex %zu", i);
    //         return false;
    //     }
    //     if (!cgltf_accessor_read_uint(joint_ids_accessor, i, (cgltf_uint*)&transfer_buffer_mapped[i].joint_ids, 4))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read joint IDs for vertex %zu", i);
    //         return false;
    //     }
    //     if (!cgltf_accessor_read_float(joint_weights_accessor, i, &transfer_buffer_mapped[i].weights, 4))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read joint weights for vertex %zu", i);
    //         return false;
    //     }
    // }

    size_t unpacked_indices_count = cgltf_accessor_unpack_indices(index_accessor, transfer_buffer_mapped + vertex_data_size, sizeof(Uint16), mesh.index_count);
    if (unpacked_indices_count != mesh.index_count)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error unpacking gltf primitive indices: unexpected index_count (unpacked %zu, expected %u).", unpacked_indices_count, mesh.index_count);
        return false;
    }

    SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

    // TODO emissive maps
    // (use SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM for non-color maps)
    char* texture_diffuse_uri = NULL;
    char* texture_metallic_roughness_uri = NULL;
    char* texture_normal_uri = NULL;
    if (primitive->material && primitive->material->has_pbr_metallic_roughness)
    {
        cgltf_texture* base_color_texture = primitive->material->pbr_metallic_roughness.base_color_texture.texture;
        if (base_color_texture && base_color_texture->image && base_color_texture->image->uri)
        {
            texture_diffuse_uri = base_color_texture->image->uri;
        }
        cgltf_texture* metallic_roughness_texture = primitive->material->pbr_metallic_roughness.metallic_roughness_texture.texture;
        if (metallic_roughness_texture && metallic_roughness_texture->image && metallic_roughness_texture->image->uri)
        {
            texture_metallic_roughness_uri = metallic_roughness_texture->image->uri;
        }
    }
    if (primitive->material && primitive->material->normal_texture.texture)
    {
        cgltf_texture* normal_texture = primitive->material->normal_texture.texture;
        if (normal_texture && normal_texture->image && normal_texture->image->uri)
        {
            texture_normal_uri = normal_texture->image->uri;
        }
    }

    // texture_diffuse_uri = "white.png";
    texture_metallic_roughness_uri = "orange.png";
    texture_normal_uri = "default_normal.png";

    SDL_Surface* texture_diffuse_surface = LoadImage(texture_diffuse_uri);
    if (!texture_diffuse_surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load diffuse texture from URI: %s", texture_diffuse_uri);
        return false;
    }
    SDL_Surface* texture_metallic_roughness_surface = LoadImage(texture_metallic_roughness_uri);
    if (!texture_metallic_roughness_surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load metallic roughness texture from URI: %s", texture_metallic_roughness_uri);
        return false;
    }
    SDL_Surface* texture_normal_surface = NULL;
    if (texture_normal_uri)
    {
        texture_normal_surface = LoadImage(texture_normal_uri);
        if (!texture_normal_surface)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load normal texture from URI: %s", texture_normal_uri);
            return false;
        }
    }
    mesh.material.texture_diffuse = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .width = (Uint32)texture_diffuse_surface->w,
        .height = (Uint32)texture_diffuse_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET // COLOR_TARGET is needed for mipmap generation
    });
    if (mesh.material.texture_diffuse == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create main texture: %s", SDL_GetError());
        return false;
    }
    mesh.material.texture_metallic_roughness = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = (Uint32)texture_metallic_roughness_surface->w,
        .height = (Uint32)texture_metallic_roughness_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET 
    });
    if (mesh.material.texture_metallic_roughness == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create metallic-roughness texture: %s", SDL_GetError());
        return false;
    }
    mesh.material.texture_normal = SDL_CreateGPUTexture(gpu_device, &(SDL_GPUTextureCreateInfo)
    {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = (Uint32)texture_normal_surface->w,
        .height = (Uint32)texture_normal_surface->h,
        .layer_count_or_depth = 1,
        .num_levels = n_mipmap_levels, 
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET 
    });
    if (mesh.material.texture_normal == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create normal texture: %s", SDL_GetError());
        return false;
    }

    // LoadImage() guarantees the surface is SDL_PIXELFORMAT_RGBA32
    Uint32 texture_diffuse_data_size = 
        (Uint32)texture_diffuse_surface->w * texture_diffuse_surface->h * 4;
    Uint32 texture_metallic_roughness_data_size = 
        (Uint32)texture_metallic_roughness_surface->w * texture_metallic_roughness_surface->h * 4;
    Uint32 texture_normal_data_size =
        (Uint32)texture_normal_surface->w * texture_normal_surface->h * 4;
    Uint32 texture_data_size = texture_diffuse_data_size + texture_metallic_roughness_data_size + texture_normal_data_size;
    
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

    SDL_memcpy(texture_transfer_mapped, texture_diffuse_surface->pixels, texture_diffuse_data_size);
    SDL_memcpy(texture_transfer_mapped + texture_diffuse_data_size, texture_metallic_roughness_surface->pixels, texture_metallic_roughness_data_size);
    SDL_memcpy(texture_transfer_mapped + texture_diffuse_data_size + texture_metallic_roughness_data_size, texture_normal_surface->pixels, texture_normal_data_size);
    
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
            .buffer = mesh.vertex_buffer, 
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
            .buffer = mesh.index_buffer,
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
            .pixels_per_row = (Uint32)texture_diffuse_surface->w,
            .rows_per_layer = (Uint32)texture_diffuse_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = mesh.material.texture_diffuse,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_diffuse_surface->w,
            .h = (Uint32)texture_diffuse_surface->h,
            .d = 1
        },
        false
    );
    SDL_UploadToGPUTexture
    (
        copy_pass,
        &(SDL_GPUTextureTransferInfo)
        {
            .transfer_buffer = texture_transfer_buffer,
            .offset = texture_diffuse_data_size,
            .pixels_per_row = (Uint32)texture_metallic_roughness_surface->w,
            .rows_per_layer = (Uint32)texture_metallic_roughness_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = mesh.material.texture_metallic_roughness,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_metallic_roughness_surface->w,
            .h = (Uint32)texture_metallic_roughness_surface->h,
            .d = 1
        },
        false
    );
    SDL_UploadToGPUTexture
    (
        copy_pass,
        &(SDL_GPUTextureTransferInfo)
        {
            .transfer_buffer = texture_transfer_buffer,
            .offset = texture_diffuse_data_size + texture_metallic_roughness_data_size,
            .pixels_per_row = (Uint32)texture_normal_surface->w,
            .rows_per_layer = (Uint32)texture_normal_surface->h
        },
        &(SDL_GPUTextureRegion)
        {
            .texture = mesh.material.texture_normal,
            .mip_level = 0,
            .layer = 0,
            .x = 0, .y = 0, .z = 0,
            .w = (Uint32)texture_normal_surface->w,
            .h = (Uint32)texture_normal_surface->h,
            .d = 1
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    
    if (n_mipmap_levels > 1) 
    {
        SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, mesh.material.texture_diffuse);
        SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, mesh.material.texture_metallic_roughness);
        SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, mesh.material.texture_normal);
    }
    
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu_device, texture_transfer_buffer);
    SDL_DestroySurface(texture_diffuse_surface);
    SDL_DestroySurface(texture_metallic_roughness_surface);
    SDL_DestroySurface(texture_normal_surface);

    if (model_type == MODEL_TYPE_BONE_ANIMATED || model_type == MODEL_TYPE_BONE_ANIMATED_MIXAMO)
    {
        Model_BoneAnimated model_bone_animated = {0};
        glm_mat4_identity(model_bone_animated.model.model_matrix);
        model_bone_animated.model.mesh = mesh;
        model_bone_animated.animation_rig = animation_rig;
        Array_Append(models_bone_animated, model_bone_animated);
    }
    else
    {
        Model new_model = {0};
        glm_mat4_identity(new_model.model_matrix);
        new_model.mesh = mesh;
        Array_Append(models_unanimated, new_model);
    }
    SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded model: %s", node->name);

    return true;
}

bool Model_Load_Collider(cgltf_data* gltf_data, cgltf_node* node)
{
    if (node->mesh->primitives_count != 1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Expected mesh with one primitive. Found %zu primitives in node: %s.", node->mesh->primitives_count, node->name);
        return false;
    }

    cgltf_primitive* primitive = node->mesh->primitives;
    if (primitive->type != cgltf_primitive_type_triangles)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: gltf primitive has type %d, expected type %d (triangles).", primitive->type, cgltf_primitive_type_triangles);
        return false;
    }

    cgltf_accessor* position_accessor = NULL;

    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_attribute* attr = &(primitive->attributes[i]);
        if (attr->type == cgltf_attribute_type_position)
        {
            position_accessor = attr->data;
        }
    }

    if (!position_accessor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Primitive is missing attribute: POSITION.");
        return false;
    }

    if (position_accessor->component_type != cgltf_component_type_r_32f) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Expected POSITION to be Float32. Current gltf loader does not support automatic conversion.");
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

    int index_count = (int)index_accessor->count;

    const uint8_t* pos_data_base = cgltf_buffer_view_data(position_accessor->buffer_view);
    if (pos_data_base == NULL) 
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get position data buffer view.");
        return false;
    }
    pos_data_base += position_accessor->offset;

    Uint16 indices[index_count];

    size_t unpacked_indices_count = cgltf_accessor_unpack_indices(index_accessor, indices, sizeof(Uint16), index_count);
    if (unpacked_indices_count != index_count)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error unpacking gltf primitive indices: unexpected index_count (unpacked %zu, expected %u).", unpacked_indices_count, index_count);
        return false;
    }

    for (int i = 0; i < index_count; i +=3)
    {
        Uint16 index0 = indices[i];
        Uint16 index1 = indices[i + 1];
        Uint16 index2 = indices[i + 2];

        Collider collider;
        
        const void* src_pos0 = pos_data_base + index0 * position_accessor->stride;
        memcpy(&collider.tri.a, src_pos0, sizeof(float) * 3);
        const void* src_pos1 = pos_data_base + index1 * position_accessor->stride;
        memcpy(&collider.tri.b, src_pos1, sizeof(float) * 3);
        const void* src_pos2 = pos_data_base + index2 * position_accessor->stride;
        memcpy(&collider.tri.c, src_pos2, sizeof(float) * 3);

        AABBFromTri(collider.tri, collider.aabb);
        
        vec3 ba;
        glm_vec3_sub(collider.tri.b, collider.tri.a, ba);
        vec3 ca;
        glm_vec3_sub(collider.tri.c, collider.tri.a, ca);
        vec3 n_raw;
        glm_vec3_cross(ba, ca, n_raw);
        glm_vec3_normalize_to(n_raw, collider.normal);
        
        Array_Append(colliders, collider);
    }

    return true;
}

// TODO modularize free funtions for mesh, material, animation rig, model, etc.

void Model_Free(Model* model)
{
    SDL_ReleaseGPUBuffer(gpu_device, model->mesh.vertex_buffer);
    SDL_ReleaseGPUBuffer(gpu_device, model->mesh.index_buffer);
    SDL_ReleaseGPUTexture(gpu_device, model->mesh.material.texture_diffuse);
    SDL_memset(model, 0, sizeof(Model));
}

void Model_BoneAnimated_Free(Model_BoneAnimated* model)
{
    SDL_ReleaseGPUBuffer(gpu_device, model->model.mesh.vertex_buffer);
    SDL_ReleaseGPUBuffer(gpu_device, model->model.mesh.index_buffer);
    SDL_ReleaseGPUTexture(gpu_device, model->model.mesh.material.texture_diffuse);
    SDL_free(model->animation_rig.joints);
    for (size_t i = 0; i < model->animation_rig.num_skeletal_animations; ++i)
    {
        SDL_free(model->animation_rig.skeletal_animations[i].key_frame_times);
        SDL_free(model->animation_rig.skeletal_animations[i].joint_updates);
    }
    SDL_free(model->animation_rig.skeletal_animations);
    SDL_memset(model, 0, sizeof(Model_BoneAnimated));
}

static void Model_CalculateJointMatrices(Joint* joint, mat4 parent_global_transform, Uint8* joint_matrices_out, Joint* root_joint) 
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
        Model_CalculateJointMatrices(&root_joint[joint->children[i]], global_transform, joint_matrices_out, root_joint);
    }
}

bool Model_JointMat_UpdateAndUpload()
{
    SDL_GPUCommandBuffer* command_buffer_joint_matrix = SDL_AcquireGPUCommandBuffer(gpu_device);
    {
        void* transfer_buffer_mapped = SDL_MapGPUTransferBuffer(gpu_device, joint_matrix_transfer_buffer, true);
        if (!transfer_buffer_mapped) 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
            return false;
        }
        
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer_joint_matrix);
        size_t current_offset_bytes = 0;

        for (size_t i = 0; i < Array_Len(models_bone_animated); i++)
        {
            // update animations

            Model_BoneAnimated* model = &models_bone_animated[i];

            model->animation_rig.animation_progress += delta_time;
            
            Animation_Skeletal* animation = &model->animation_rig.skeletal_animations[model->animation_rig.active_animation_index];
            
            Uint16 frame_index = 0;
            bool animation_finished = true;

            for (size_t ii = 0; ii < animation->num_key_frames; ii++)
            {
                if (animation->key_frame_times[ii] > model->animation_rig.animation_progress)
                {
                    animation_finished = false;
                    break;
                }
                frame_index = (Uint16)ii;
            }

            if (animation_finished && animation->is_looping)
            {
                model->animation_rig.animation_progress = 0.0f;
                frame_index = 0;
            }
            else if (animation_finished)
            {
                // TODO this needs some kind of state machine to determine what animation to play next
                // for now, just reset to the first frame of the first animation
                model->animation_rig.active_animation_index = 0;
                animation = &model->animation_rig.skeletal_animations[model->animation_rig.active_animation_index];
                model->animation_rig.animation_progress = 0.0f;
                frame_index = 0;
            }
            
            float interpolant = (model->animation_rig.animation_progress - animation->key_frame_times[frame_index]) / 
                (animation->key_frame_times[frame_index + 1] - animation->key_frame_times[frame_index]);

            for (size_t ii = 0; ii < animation->num_joint_updates_per_frame; ii++)
            {
                Joint_Update* prev_joint_update = &animation->joint_updates[ii + frame_index * animation->num_joint_updates_per_frame];
                Joint_Update* next_joint_update = &animation->joint_updates[ii + (frame_index + 1) * animation->num_joint_updates_per_frame];
                switch (prev_joint_update->joint_update_type)
                {
                    case JOINT_UPDATE_TYPE_TRANSLATION:
                        glm_vec3_lerp(prev_joint_update->translation, next_joint_update->translation, interpolant, model->animation_rig.joints[prev_joint_update->joint_index].translation);
                        break;
                    case JOINT_UPDATE_TYPE_ROTATION:
                        glm_quat_slerp(prev_joint_update->rotation, next_joint_update->rotation, interpolant, model->animation_rig.joints[prev_joint_update->joint_index].rotation);
                        break;
                    case JOINT_UPDATE_TYPE_SCALE:
                        glm_vec3_lerp(prev_joint_update->scale, next_joint_update->scale, interpolant, model->animation_rig.joints[prev_joint_update->joint_index].scale);
                        break;
                    default:
                        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Unknown joint update type: %d", prev_joint_update->joint_update_type);
                }
            }

            // update joint matrices

            models_bone_animated[i].animation_rig.storage_buffer_offset_bytes = current_offset_bytes;

            // for mixamo models, the first joint should also be the root node 
            // in the future this may need to be determined by checking the joint hierarchy
            
            Model_CalculateJointMatrices
            (
                models_bone_animated[i].animation_rig.joints, // root joint
                models_bone_animated[i].animation_rig.armature_correction_matrix, // parent transform
                (Uint8*)transfer_buffer_mapped + current_offset_bytes, models_bone_animated[i].animation_rig.joints // root joint
            );

            current_offset_bytes += models_bone_animated[i].animation_rig.num_joints * sizeof(mat4);
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
            
        SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);
        
        SDL_UnmapGPUTransferBuffer(gpu_device, joint_matrix_transfer_buffer);
        SDL_EndGPUCopyPass(copy_pass);
        
    }
    SDL_SubmitGPUCommandBuffer(command_buffer_joint_matrix);

    return true;
}