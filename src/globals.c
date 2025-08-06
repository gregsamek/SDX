#include <SDL3/SDL.h>

#include "globals.h"

float mouse_sensitivity = 0.1f;
float movement_speed = 10.0f; // Units per second

vec3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};

SDL_Window* window;
SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
int window_width, window_height;
bool window_resized = false;

Uint64 last_ticks = 0;
Uint64 current_ticks = 0;
Uint64 performance_frequency = 0;
float delta_time = 0.0f;

const char* base_path = NULL;

bool* keyboard_state = NULL;
bool is_mouse_captured = false;
InputState input_state = InputState_DEFAULT;

Array_Model models_unanimated = {0};
Array_Model_BoneAnimated models_bone_animated = {0};

Array_Sprite sprites = {0};

float input_deadzone_squared = 0.001f;
Camera camera =
{
    .position = {0.0f, 2.5f, -3.0f},
    .forward = {0},
    .up = {0},
    .right = {0},
    .yaw = 90.0f,
    .pitch = -30.0f,
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
double target_frame_time = 1.0 / 60.0;
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
Uint32 virtual_screen_texture_height = 240;
SDL_GPUSampler* default_texture_sampler = NULL;
SDL_GPUBuffer* joint_matrix_storage_buffer = NULL;
SDL_GPUTransferBuffer* joint_matrix_transfer_buffer = NULL;