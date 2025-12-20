#ifndef MODEL_H
#define MODEL_H

#include <SDL3/SDL.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"
#include "../external/cgltf.h"

#include "helper.h"

Enum (Uint8, Model_Type)
{
    MODEL_TYPE_UNKNOWN = 0,
	MODEL_TYPE_UNRENDERED, // mesh that may have use in Blender, but not the game
	MODEL_TYPE_UNANIMATED,
	MODEL_TYPE_BONE_ANIMATED_MIXAMO, // TODO how to handle some of these not being mutually exclusive?
	MODEL_TYPE_BONE_ANIMATED,
	MODEL_TYPE_RIGID_ANIMATED,
	MODEL_TYPE_INSTANCED,
	MODEL_TYPE_COLLIDER,
};

Struct (TransformsUBO)
{
    mat4 mvp; // VP * M
    mat4 mv;  // V * M
	mat4 mvp_light; // light VP * M
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
	mat4 normal; // upper-left 3x3 is the normal matrix, rest identity
#endif
};

Struct (Vertex_Position)
{
	float x, y, z;
};

Struct (Vertex_PositionNormal)
{
	float x, y, z;  // position
	float nx, ny, nz; // normal
};

Struct (Vertex_PositionTexture)
{
	float x, y, z;
	float u, v;
};

Struct (Vertex_PBR)
{
    float x, y, z;  // position
    float nx, ny, nz; // normal
    float u, v;     // texcoord
	float tx, ty, tz, tw; // for normal mapping
};

#define MAX_JOINTS_PER_VERTEX 4

Struct (Vertex_BoneAnimated)
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float tx, ty, tz, tw;
	Uint8 joint_ids[MAX_JOINTS_PER_VERTEX]; 
	float weights[MAX_JOINTS_PER_VERTEX];
};

// in the future may add emission, masks and blends
Struct (Material)
{
	SDL_GPUTexture* texture_diffuse;
	SDL_GPUTexture* texture_normal;
	SDL_GPUTexture* texture_metallic_roughness;
};

// Mesh is equivalent to a GLTF "Primitive"
// aka geometry that can be rendered with a single draw call
Struct (Mesh)
{
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
	Material material;
	Uint32 index_count;
};

Struct (Node)
{
	mat4 local_transform;
	Mesh mesh;
	struct Node* children; // array of child nodes
	Uint8 num_children;
	Uint8 _padding[7];
};

Struct (Entity)
{
	Uint32 id;
	vec3 position;
	versor rotation;
};

Struct (Model)
{
	mat4 model_matrix;
	Mesh mesh;
};

// TODO morph targets?

#define MAX_CHILDREN_PER_JOINT 3

Struct (Joint)
{
	mat4 inverse_bind_matrix;
	vec3 translation;
	versor rotation;
	vec3 scale;
	Uint8 num_children;
	Uint8 children[MAX_CHILDREN_PER_JOINT];
	Uint8 _padding[4]; // mat4 is 16 byte aligned; this makes Joint 112 bytes
};

Enum (Uint8, Joint_Update_Type)
{
    JOINT_UPDATE_TYPE_UNKNOWN = 0,
	JOINT_UPDATE_TYPE_TRANSLATION,
	JOINT_UPDATE_TYPE_ROTATION,
	JOINT_UPDATE_TYPE_SCALE,
};

Struct (Joint_Update)
{
	// using float arrays here bc glm `versor` is 16 byte aligned, 
	// which would make this struct way bigger than necessary
	union
	{
		float translation[3];
		float rotation[4];
		float scale[3];
	};
	Joint_Update_Type joint_update_type;
	Uint8 joint_index;
	Uint8 _padding[2];
};

Enum (Uint8, Animation_Skeletal_ID)
{
	ANIMATION_SKELETAL_ID_UNKNOWN = 0,
	ANIMATION_SKELETAL_ID_IDLE,
	ANIMATION_SKELETAL_ID_WALK,
};

Struct (Animation_Skeletal)
{
	float* key_frame_times;
    Joint_Update* joint_updates;
	Uint16 num_key_frames;
    Uint16 num_joint_updates_per_frame;
	Animation_Skeletal_ID animation_id;
	bool is_looping;
	Uint8 _padding[2];
};

Struct (Animation_Rig)
{
	mat4 armature_correction_matrix;
	Joint* joints;
	Animation_Skeletal* skeletal_animations;
	Uint32 storage_buffer_offset_bytes;
	float animation_progress;
	Uint8 num_joints;
	Uint8 num_skeletal_animations;
	Uint8 active_animation_index;
};

Struct (Model_BoneAnimated)
{
	Model model;
	Animation_Rig animation_rig;
};

bool Model_Load_AllScenes(void);
bool Model_Load_Scene(const char* filename);
bool Model_Load(cgltf_data* gltf_data, cgltf_node* node);
void Model_Free(Model* model);
void Model_BoneAnimated_Free(Model_BoneAnimated* model);
bool Model_JointMat_UpdateAndUpload();
bool Model_Load_Collider(cgltf_data* gltf_data, cgltf_node* node);

#endif // MODEL_H