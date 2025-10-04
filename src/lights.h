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
} Light_Directional;

typedef struct
{
    vec3 location;
    float attenuation_constant; // ???
    vec3 color;
} Light_Point;

typedef struct
{
    vec3 position;
    float cutoff;  // don't pass angle; pass SDL_cosf(glm_rad(angle))
    vec3 direction;
} Light_Spotlight;


#endif // LIGHTS_H