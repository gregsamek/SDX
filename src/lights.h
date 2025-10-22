#ifndef LIGHTS_H
#define LIGHTS_H

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

Struct (Light_Spotlight)
{
    vec3 position;
    float attenuation_constant_linear; 
    vec3 color;
    float attenuation_constant_quadratic;
    vec3 direction;
    float cutoff_inner;  // don't pass angle; pass SDL_cosf(glm_rad(angle))
    float cutoff_outer;
    float _padding[3]; // pad to vec4 size
};

Struct (Light_Hemisphere)
{
    vec3 up_viewspace;
    float _padding;
    vec3 color_sky;
    float _padding2;
    vec3 color_ground;
    float _padding3;
};

bool Lights_StorageBuffer_UpdateAndUpload();


#endif // LIGHTS_H