#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>

#include "events.h"
#include "model.h"
#include "lights.h"

Struct (UBO_SSAO)
{
    mat4 projection_matrix; // View -> Clip. LH, depth 0..1
    Uint32 settings_render;
    vec2 screen_size;
    float radius; // TODO scale radius with depth?
    float bias;
    float intensity;
    float power;
    Uint32 kernel_size;
};

Struct (UBO_Main_Frag)
{
    Light_Directional light_directional;
    Light_Hemisphere light_hemisphere;
    Shadow_Settings shadow_settings;
    vec2 inverse_screen_resolution;
    Uint32 settings_render;
    float _padding;
};

Struct (UBO_Fog_Frag)
{
    vec3   color;          // linear space
    float  density;        // for exp/exp2; e.g. 0.02

    float  start;          // for linear fog
    float  end;            // for linear fog
    int    mode;           // 0=Linear, 1=Exp, 2=Exp2
    int    depth_is_view_z;      // 1 if texture stores camera-space z (positive = -z_view), 0 if it stores Euclidean distance

    // Height fog (optional)
    float  height_fog_enable;   // 0 or 1
    float  fog_height;         // world-space height of fog layer
    float  height_falloff;     // e.g. 0.1–2.0
    float  _padding0;

    mat4 inv_proj_mat;         // inverse projection (for view-ray)
    mat4 inv_view_mat;         // inverse view (view->world), for height fog
};

Struct (UBO_SSAOUpsample)
{
    float sigma_spatial; // in low-res pixel units (e.g. 1.0)
    float sigma_depth;   // in view-space z units (e.g. 1.0 .. 3.0 depending on your scale)
    float sigma_normal;  // angular softness via (1 - dot(n)) scaling (e.g. 0.1 .. 0.3)
    float normal_power;  // additional sharpening via pow(dot, normalPower) (e.g. 8 .. 32). Set to 1 to disable.
};

Struct (UBO_Bloom_Threshold)
{
    float  threshold;    // e.g., 1.0–2.0 in HDR linear (after exposure)
    float  soft_knee;    // [0..1], 0 = hard, ~0.5 is common
    Uint32 use_maxRGB;   // 1 = maxRGB metric, 0 = luminance
    float  exposure;
};

Struct (UBO_Bloom_Downsample)
{
    float radius;
    float tap_bias;
    Uint32 _padding[2];
};

bool Render_LoadRenderSettings();
bool Render_Init();
bool Render();

#endif // RENDER_H