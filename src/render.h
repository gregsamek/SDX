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
    float kernel_size;
};

Struct (UBO_Main_Frag)
{
    Light_Directional light_directional;
    Light_Hemisphere light_hemisphere;
    Shadow_Settings shadow_settings;
    vec2 inverse_ssao_texture_size;
    Uint32 settings_render;
    float _padding;
};

bool Render_LoadRenderSettings();
bool Render_Init();
bool Render();

#endif // RENDER_H