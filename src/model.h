#ifndef MODEL_H
#define MODEL_H

#include <SDL3/SDL.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"

typedef Uint8 Model_Type; enum
{
    MODEL_TYPE_UNKNOWN = 0,
	MODEL_TYPE_UNRENDERED,
	MODEL_TYPE_UNANIMATED,
	MODEL_TYPE_BONE_ANIMATED,
	MODEL_TYPE_RIGID_ANIMATED,
	MODEL_TYPE_INSTANCED,
};

typedef struct TransformsUBO
{
    mat4 mvp; // VP * M
    mat4 mv;  // V * M
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
	mat4 normal; // upper-left 3x3 is the normal matrix, rest identity
#endif
} TransformsUBO;

typedef struct Vertex_Position
{
	float x, y, z;
} Vertex_Position;

typedef struct Vertex_PositionNormal
{
	float x, y, z;  // position
	float nx, ny, nz; // normal
} Vertex_PositionNormal;

typedef struct Vertex_PositionTexture
{
	float x, y, z;
	float u, v;
} Vertex_PositionTexture;

typedef struct Vertex_PositionNormalTexture
{
    float x, y, z;  // position
    float nx, ny, nz; // normal
    float u, v;     // texcoord
} Vertex_PositionNormalTexture;

#define MAX_JOINTS_PER_VERTEX 4

typedef struct Vertex_BoneAnimated
{
	float x, y, z;
	float u, v;
	Uint8 joint_ids[MAX_JOINTS_PER_VERTEX]; 
	float weights[MAX_JOINTS_PER_VERTEX];
} Vertex_BoneAnimated;

typedef struct Material
{
	SDL_GPUTexture* diffuse_texture;
	SDL_GPUSampler* sampler;
	// no pbr for now
} Material;

// TODO morph targets, weights, skins

// Mesh is equivalent to a GLTF "Primitive"
// aka geometry that can be rendered with a single draw call
typedef struct Mesh
{
	SDL_GPUBuffer* vertex_buffer; // positions, texcoords
	SDL_GPUBuffer* index_buffer;
	Material material;
} Mesh;

typedef struct Node
{
	Mesh mesh;
	mat4 local_transform;
	struct Node* children; // array of child nodes
} Node;

typedef struct Entity
{
	Uint32 id;
	vec3 position;
	versor rotation;
} Entity;

typedef struct Model
{
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
	SDL_GPUTexture* texture;
	Uint32 index_count;
} Model;

typedef struct Array_Model
{
    Model* arr;
    Uint32 len;
    Uint32 cap;
} Array_Model;

#define MAX_CHILDREN_PER_JOINT 3

typedef struct Joint
{
	mat4 inverse_bind_matrix;
	vec3 translation;
	versor rotation;
	vec3 scale;
	Uint8 num_children;
	Uint8 children[MAX_CHILDREN_PER_JOINT];
	Uint8 _padding[4]; // mat4 is 16 byte aligned; this makes Joint 112 bytes
} Joint;

typedef Uint8 Joint_Update_Type; enum
{
    JOINT_UPDATE_TYPE_UNKNOWN = 0,
	JOINT_UPDATE_TYPE_TRANSLATION,
	JOINT_UPDATE_TYPE_ROTATION,
	JOINT_UPDATE_TYPE_SCALE,
};

typedef struct Joint_Update
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
} Joint_Update;

typedef Uint8 Animation_Skeletal_ID; enum
{
	ANIMATION_SKELETAL_ID_UNKNOWN = 0,
	ANIMATION_SKELETAL_ID_IDLE,
	ANIMATION_SKELETAL_ID_WALK,
};

typedef struct Animation_Skeletal
{
	float* key_frame_times;
    Joint_Update* joint_updates;
	Uint16 num_key_frames;
    Uint16 num_joint_updates_per_frame;
	Animation_Skeletal_ID animation_id;
	bool is_looping;
	Uint8 _padding[2];
} Animation_Skeletal;

typedef struct Model_BoneAnimated
{
	mat4 model_matrix;
	mat4 armature_correction_matrix;
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
	SDL_GPUTexture* texture;
	Joint* joints;
	Animation_Skeletal* skeletal_animations;
	Uint32 index_count;
	Uint32 storage_buffer_offset_bytes;
	float animation_progress;
	Uint8 num_joints;
	Uint8 num_skeletal_animations;
	Uint8 active_animation_index;
	Uint8 _padding[9];
} Model_BoneAnimated;

typedef struct Array_Model_BoneAnimated
{
    Model_BoneAnimated* arr;
    Uint32 len;
    Uint32 cap;
} Array_Model_BoneAnimated;

bool Model_LoadAllModels(void);
bool Model_Load(const char* filename);
void Model_Free(Model* model);
void Model_BoneAnimated_Free(Model_BoneAnimated* model);
bool Model_JointMat_UpdateAndUpload();

#endif // MODEL_H