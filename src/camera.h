#ifndef CAMERA_H
#define CAMERA_H

#include "helper.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"

// #define CAMERA_FIXED_HORIZONTAL_FOV

Struct (Camera)
{
    mat4 view_matrix;
    mat4 projection_matrix;
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
};

void Camera_UpdateDirection(Camera* camera);
void Camera_MoveNoClip(Camera* camera);
void Camera_UpdateMatrices(Camera* camera);
void Camera_Log();

#endif // CAMERA_H