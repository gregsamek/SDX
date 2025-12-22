#include <SDL3/SDL.h>

#include "globals.h"

Settings_Render settings_render = 
    SETTINGS_RENDER_ENABLE_FOG | SETTINGS_RENDER_ENABLE_SSAO | 
    SETTINGS_RENDER_ENABLE_SHADOWS | SETTINGS_RENDER_USE_LINEAR_FILTERING | 
    SETTINGS_RENDER_UPSCALE_SSAO | SETTINGS_RENDER_ENABLE_BLOOM;

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
InputState input_state = InputState_FIRSTPERSONCONTROLLER;
float mouse_sensitivity = 0.1f;
float input_deadzone_squared = 0.001f;
float camera_noclip_movementSpeed = 10.0f; // Units per second
Camera camera_noClip =
{
    .position = {6.0f, 2.5f, 5.5f},
    .forward = {0},
    .up = {0},
    .right = {0},
    .yaw = 222.0f,
    .pitch = -15.0f,
    .fov = 70.0f,
    .near_plane = 0.1f,
    .far_plane = 200.0f,
    .view_projection_matrix = {0}
};
Camera* activeCamera = NULL;
float mouse_xrel = 0.0f;
float mouse_yrel = 0.0f;
bool mouse_invertX = false;
bool mouse_invertY = false;

Player player = {0};

Collider Array colliders = NULL;
Trigger Array triggers = NULL;

Model Array models_unanimated = NULL;
Model_BoneAnimated Array models_bone_animated = NULL;

Light_Directional light_directional = {0};
Light_Hemisphere light_hemisphere = {0};
Light_Spot Array lights_spot = NULL;

Sprite Array sprites = NULL;

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
SDL_GPUSampleCount msaa_level = SDL_GPU_SAMPLECOUNT_1;
Uint32 n_mipmap_levels = 6;
SDL_GPUDevice* gpu_device = NULL;

SDL_GPUGraphicsPipeline* pipeline_prepass_unanimated = NULL;
SDL_GPUGraphicsPipeline* pipeline_ssao = NULL;
SDL_GPUGraphicsPipeline* pipeline_unanimated = NULL;
SDL_GPUGraphicsPipeline* pipeline_bone_animated = NULL;
// SDL_GPUGraphicsPipeline* pipeline_rigid_animated = NULL;
// SDL_GPUGraphicsPipeline* pipeline_instanced = NULL;
SDL_GPUGraphicsPipeline* pipeline_text = NULL;
SDL_GPUGraphicsPipeline* pipeline_swapchain = NULL;
SDL_GPUGraphicsPipeline* pipeline_sprite = NULL;
SDL_GPUGraphicsPipeline* pipeline_fog = NULL;

SDL_GPUComputePipeline* pipeline_prepass_downsample = NULL;
SDL_GPUComputePipeline* pipeline_ssao_upsample = NULL;
SDL_GPUComputePipeline* pipeline_bloom_threshold = NULL;
SDL_GPUComputePipeline* pipeline_bloom_downsample = NULL;
SDL_GPUComputePipeline* pipeline_bloom_upsample = NULL;
SDL_GPUComputePipeline* pipeline_gaussian_blur = NULL;

SDL_GPUTexture* prepass_texture_msaa = NULL;
SDL_GPUTexture* prepass_texture = NULL;
SDL_GPUTexture* prepass_texture_half = NULL;
SDL_GPUTexture* ssao_texture = NULL;
SDL_GPUTexture* ssao_texture_upsampled = NULL;
SDL_GPUTexture* fog_texture = NULL;

#if DUAL_KAWASE_BLOOM
SDL_GPUTexture* bloom_textures_downsampled[MAX_BLOOM_LEVELS] = { 0 };
SDL_GPUTexture* bloom_textures_upsampled[MAX_BLOOM_LEVELS] = { 0 };
#endif
SDL_GPUTexture* bloom_textures[2] = { 0 };

SDL_GPUTexture* depth_texture = NULL;
SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_INVALID;
SDL_GPUTexture* msaa_texture = NULL;
SDL_GPUTexture* virtual_screen_texture = NULL;
Uint32 virtual_screen_texture_width = 0;
Uint32 virtual_screen_texture_height = 360;
SDL_GPUSampler* sampler_albedo = NULL;

SDL_GPUBuffer* joint_matrix_storage_buffer = NULL;
SDL_GPUTransferBuffer* joint_matrix_transfer_buffer = NULL;
SDL_GPUBuffer* lights_storage_buffer = NULL;
SDL_GPUTransferBuffer* lights_transfer_buffer = NULL;

SDL_GPUTexture* shadow_map_texture = NULL;
SDL_GPUSampler* sampler_nearest_nomips = NULL;
SDL_GPUSampler* sampler_linear_nomips = NULL;
SDL_GPUGraphicsPipeline* pipeline_shadow_depth = NULL;
SDL_GPUTextureFormat depth_sample_texture_format = SDL_GPU_TEXTUREFORMAT_INVALID;
Uint32 SHADOW_MAP_SIZE = 1024;
float SHADOW_ORTHO_HALF_WIDTH = 30.0f;
float SHADOW_NEAR = 0.1f;
float SHADOW_FAR  = 150.0f;
float SHADOW_BIAS = 0.00002f;
float SHADOW_PCF_RADIUS = 1.5f; // in texels
mat4 light_view_matrix = {0};
mat4 light_proj_matrix = {0};
mat4 light_viewproj_matrix = {0};
Shadow_Settings shadow_settings = {0};