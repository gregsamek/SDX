#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "model.h"
#include "camera.h"
#include "events.h"
#include "text.h"
#include "sprite.h"
#include "lights.h"

#include "array.h"

// TODO I should probably move this into a dedicated header that can be shared with shaders
Enum (Uint32, Settings_Render)
{
    SETTINGS_RENDER_NONE                    = 0,
    SETTINGS_RENDER_SHOW_DEBUG_TEXTURE      = 1 << 0,
    SETTINGS_RENDER_LINEARIZE_DEBUG_TEXTURE = 1 << 1,
    SETTINGS_RENDER_ENABLE_SSAO             = 1 << 2,
    SETTINGS_RENDER_ENABLE_SHADOWS          = 1 << 3,
    SETTINGS_RENDER_USE_LINEAR_FILTERING    = 1 << 4,
    SETTINGS_RENDER_ENABLE_FOG              = 1 << 5
};

extern Settings_Render settings_render;

extern float mouse_sensitivity;
extern float movement_speed;

extern vec3 WORLD_UP_VECTOR;

extern SDL_Window* window;
extern SDL_WindowFlags window_flags;
extern int window_width, window_height;
extern bool renderer_needs_to_be_reinitialized;

#define MAXIMUM_DELTA_TIME 0.1f
extern Uint64 last_ticks;
extern Uint64 current_ticks;
extern Uint64 performance_frequency;
extern float delta_time;
extern double average_frame_rate;

#define MAXIMUM_URI_LENGTH 512
extern const char* base_path;

extern bool* keyboard_state;
extern bool is_mouse_captured;
extern InputState input_state;

extern Model Array models_unanimated;
extern Model_BoneAnimated Array models_bone_animated;

extern Light_Directional light_directional;
extern Light_Hemisphere light_hemisphere;
extern Light_Spot Array lights_spot;

extern Sprite Array sprites;

extern float input_deadzone_squared;
extern Camera camera;

extern TTF_Font *font;
extern TTF_TextEngine *textEngine;
extern Text_Renderable text_renderable;
extern Text_Vertex* text_vertices_buffer_cpu;
extern int* text_indices_buffer_cpu;
extern SDL_GPUTransferBuffer* text_transfer_buffer;

// GPU
#define MAX_TOTAL_JOINTS_TO_RENDER 99 // This determines the size of the joint matrix storage buffer
#define MAX_TOTAL_LIGHTS 16
extern SDL_GPUSwapchainComposition swapchain_composition;
extern SDL_GPUPresentMode swapchain_present_mode;
extern double minimum_frame_time;
extern SDL_GPUSampleCount msaa_level;
extern Uint32 n_mipmap_levels;
extern SDL_GPUDevice* gpu_device;

extern SDL_GPUGraphicsPipeline* pipeline_prepass_unanimated;
extern SDL_GPUGraphicsPipeline* pipeline_ssao;
extern SDL_GPUGraphicsPipeline* pipeline_unanimated;
extern SDL_GPUGraphicsPipeline* pipeline_bone_animated;
extern SDL_GPUGraphicsPipeline* pipeline_rigid_animated;
extern SDL_GPUGraphicsPipeline* pipeline_instanced;
extern SDL_GPUGraphicsPipeline* pipeline_fullscreen_quad;
extern SDL_GPUGraphicsPipeline* pipeline_text;
extern SDL_GPUGraphicsPipeline* pipeline_sprite;
extern SDL_GPUGraphicsPipeline* pipeline_fog;

extern SDL_GPUTexture* prepass_texture_msaa;
extern SDL_GPUTexture* prepass_texture;
extern SDL_GPUTexture* prepass_texture_half;
extern SDL_GPUTexture* ssao_texture;
extern SDL_GPUTexture* fog_texture;
extern SDL_GPUTexture* depth_texture;
extern SDL_GPUTextureFormat depth_texture_format;
extern SDL_GPUTexture* msaa_texture;
extern SDL_GPUTexture* virtual_screen_texture;
extern Uint32 virtual_screen_texture_width;
extern Uint32 virtual_screen_texture_height;
extern SDL_GPUSampler* default_texture_sampler;

extern SDL_GPUBuffer* joint_matrix_storage_buffer;
extern SDL_GPUTransferBuffer* joint_matrix_transfer_buffer;
extern SDL_GPUBuffer* lights_storage_buffer;
extern SDL_GPUTransferBuffer* lights_transfer_buffer;

extern SDL_GPUTexture* shadow_map_texture;
extern SDL_GPUSampler* sampler_data_texture;
extern SDL_GPUGraphicsPipeline* pipeline_shadow_depth;
extern SDL_GPUTextureFormat depth_sample_texture_format;
extern Uint32 SHADOW_MAP_SIZE;
extern float SHADOW_ORTHO_HALF_WIDTH;       // covers +-30m around focus
extern float SHADOW_NEAR;
extern float SHADOW_FAR;
extern float SHADOW_BIAS;           // constant bias in depth compare space
extern float SHADOW_PCF_RADIUS;        // in texels
extern mat4 light_view_matrix;
extern mat4 light_proj_matrix;
extern mat4 light_viewproj_matrix;
extern Shadow_Settings shadow_settings;

#endif // GLOBALS_H