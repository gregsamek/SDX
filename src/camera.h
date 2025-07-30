#ifndef CAMERA_H
#define CAMERA_H

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"

typedef struct Camera
{
    mat4 view_projection_matrix;
    vec3 position;
    vec3 forward;
    vec3 up;
    vec3 right;
    float yaw;
    float pitch;
    float fov;
    float near_plane;
    float far_plane;
    float _padding[3];
} Camera;

void Camera_Update();
void Camera_Log();

#endif // CAMERA_H