#ifndef LIGHTS_H
#define LIGHTS_H

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"
#include "helper.h"

// already a direction, so no need to calc `lightPosVS - input.PositionVS`
// no need to calc attentuation - assumed to be none
Struct (Light_Directional)
{
    vec3 direction;
    float strength; // ???
    vec3 color;
    float _padding; // pad to vec4 size
};

Struct (Light_Point)
{
    vec3 position;
    float attenuation_constant_linear; 
    vec3 color;
    float attenuation_constant_quadratic;
};

Struct (Light_Spot)
{
    vec3 position;
    float attenuation_constant_linear; 
    vec3 color;
    float attenuation_constant_quadratic;
    vec3 direction;

    // these angles are saved as SDL_cosf(glm_rad(angle in degrees))
    // note also that these are HALF-angles - from the center of the cone to the edge
    float cutoff_inner;
    float cutoff_outer;
    
    float _padding[3]; // pad to vec4 size
};

// used in hemispheric (pseudo-IBL) lighting
Struct (Light_Hemisphere)
{
    vec3 up_viewspace;
    float _padding;
    vec3 color_sky;
    float _padding2;
    vec3 color_ground;
    float _padding3;
};

Struct (ShadowUBO) 
{
    vec2 texel_size; // 1/width, 1/height
    float bias;
    float pcf_radius; // in texels
};

bool Lights_StorageBuffer_UpdateAndUpload();
void Lights_UpdateShadowMatrices_Directional(vec3 light_dir_world);
void Lights_UpdateShadowMatrices_Spot(Light_Spot* light);

#endif // LIGHTS_H