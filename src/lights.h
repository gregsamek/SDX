#ifndef LIGHTS_H
#define LIGHTS_H

#include "../external/cglm/cglm.h"

// already a direction, so no need to calc `lightPosVS - input.PositionVS`
// no need to calc attentuation - assumed to be none
typedef struct
{
    vec3 direction;
    float strength; // ???
    vec3 color;
    float padding; // pad to vec4 size
} Light_Directional;

typedef struct
{
    vec3 position;
    float attenuation_constant_linear; 
    vec3 color;
    float attenuation_constant_quadratic;
} Light_Point;

typedef struct
{
    vec3 position;
    float attenuation_constant_linear; 
    vec3 color;
    float attenuation_constant_quadratic;
    vec3 direction;
    float cutoff_inner;  // don't pass angle; pass SDL_cosf(glm_rad(angle))
    float cutoff_outer;
    float padding[3]; // pad to vec4 size
} Light_Spotlight;

bool Lights_StorageBuffer_UpdateAndUpload();


#endif // LIGHTS_H