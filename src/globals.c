#include <SDL3/SDL.h>

#include "globals.h"

bool magic_debug_mode = false;

float mouse_sensitivity = 0.1f;
float movement_speed = 10.0f; // Units per second

vec3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};

SDL_Window* window;
SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
int window_width, window_height;
bool renderer_needs_to_be_reinitialized = false;

Uint64 last_ticks = 0;
Uint64 current_ticks = 0;
Uint64 performance_frequency = 0;
float delta_time = 0.0f;
double average_frame_rate = 0.0;

const char* base_path = NULL;

bool* keyboard_state = NULL;
bool is_mouse_captured = false;
InputState input_state = InputState_DEFAULT;

Model Array models_unanimated = NULL;
Model_BoneAnimated Array models_bone_animated = NULL;

Sprite Array sprites = NULL;

float input_deadzone_squared = 0.001f;
Camera camera =
{
    .position = {6.0f, 2.5f, 5.5f},
    .forward = {0},
    .up = {0},
    .right = {0},
    .yaw = 222.0f,
    .pitch = -15.0f,
    .fov = 75.0f,
    .near_plane = 0.1f,
    .far_plane = 1000.0f,
    .view_projection_matrix = {0}
};

TTF_Font *font = NULL;
TTF_TextEngine *textEngine = NULL;
Text_Renderable text_renderable = {0};
Text_Vertex* text_vertices_buffer_cpu = NULL;
int* text_indices_buffer_cpu = NULL;
SDL_GPUTransferBuffer* text_transfer_buffer = NULL;

// GPU
SDL_GPUSwapchainComposition swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
SDL_GPUPresentMode swapchain_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
double minimum_frame_time = 1.0 / 60.0;
bool manage_frame_rate_manually = false;
SDL_GPUSampleCount msaa_level = SDL_GPU_SAMPLECOUNT_1;
Uint32 n_mipmap_levels = 6;
bool use_linear_filtering = false;
SDL_GPUDevice* gpu_device = NULL;
SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_INVALID;
SDL_GPUGraphicsPipeline* pipeline_unanimated = NULL;
SDL_GPUGraphicsPipeline* pipeline_bone_animated = NULL;
SDL_GPUGraphicsPipeline* pipeline_rigid_animated = NULL;
SDL_GPUGraphicsPipeline* pipeline_instanced = NULL;
SDL_GPUGraphicsPipeline* pipeline_text = NULL;
SDL_GPUGraphicsPipeline* pipeline_fullscreen_quad = NULL;
SDL_GPUGraphicsPipeline* pipeline_sprite = NULL;
SDL_GPUTexture* depth_texture = NULL;
SDL_GPUTexture* msaa_texture = NULL;
SDL_GPUTexture* virtual_screen_texture = NULL;
Uint32 virtual_screen_texture_width = 0;
Uint32 virtual_screen_texture_height = 360;
SDL_GPUSampler* default_texture_sampler = NULL;
SDL_GPUBuffer* joint_matrix_storage_buffer = NULL;
SDL_GPUTransferBuffer* joint_matrix_transfer_buffer = NULL;
SDL_GPUBuffer* lights_storage_buffer = NULL;
SDL_GPUTransferBuffer* lights_transfer_buffer = NULL;

SDL_GPUTexture* shadow_map_texture = NULL;
SDL_GPUSampler* shadow_sampler = NULL;
SDL_GPUGraphicsPipeline* pipeline_shadow_depth = NULL;
SDL_GPUTextureFormat shadow_map_texture_format = SDL_GPU_TEXTUREFORMAT_INVALID;
Uint32 SHADOW_MAP_SIZE = 1024;         // 1024 or 2048 works well
float SHADOW_ORTHO_HALF = 30.0f;       // covers +-30m around focus
float SHADOW_NEAR = 0.001f;
float SHADOW_FAR  = 150.0f;
float SHADOW_BIAS = 0.0015f;           // constant bias in depth compare space
float SHADOW_PCF_RADIUS = 1.5f;        // in texels
mat4 light_view_matrix = {0};
mat4 light_proj_matrix = {0};
mat4 light_viewproj_matrix = {0};
ShadowUBO shadow_ubo = {0};