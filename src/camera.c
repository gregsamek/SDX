#include "camera.h"
#include "globals.h"

static float camera_angle_adjust_speed = 80.0f;

void Camera_Update()
{
    camera.yaw += keyboard_state[SDL_SCANCODE_LEFT] * camera_angle_adjust_speed * delta_time;
    camera.yaw -= keyboard_state[SDL_SCANCODE_RIGHT] * camera_angle_adjust_speed * delta_time;
    if (camera.yaw > 360.0f) camera.yaw -= 360.0f;
    if (camera.yaw < 0.0f)   camera.yaw += 360.0f;
    
    camera.pitch += keyboard_state[SDL_SCANCODE_UP] * camera_angle_adjust_speed * delta_time;
    camera.pitch -= keyboard_state[SDL_SCANCODE_DOWN] * camera_angle_adjust_speed * delta_time;
    if (camera.pitch > 89.0f)  camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    vec3 camera_forward_new;
    camera_forward_new[0] = SDL_cosf(glm_rad(camera.yaw)) * SDL_cosf(glm_rad(camera.pitch));
    camera_forward_new[1] = SDL_sinf(glm_rad(camera.pitch));
    camera_forward_new[2] = SDL_sinf(glm_rad(camera.yaw)) * SDL_cosf(glm_rad(camera.pitch));
    glm_vec3_normalize_to(camera_forward_new, camera.forward);

    // Right is perpendicular to forward and world up
    glm_vec3_crossn(camera.forward, WORLD_UP_VECTOR, camera.right);
    // Up is perpendicular to right and forward
    glm_vec3_crossn(camera.right, camera.forward, camera.up);

    float input_forward = (int)keyboard_state[SDL_SCANCODE_W]  - (int)keyboard_state[SDL_SCANCODE_S];
    float input_right   = (int)keyboard_state[SDL_SCANCODE_A]  - (int)keyboard_state[SDL_SCANCODE_D];
    float input_up      = (int)keyboard_state[SDL_SCANCODE_E] - (int)keyboard_state[SDL_SCANCODE_Q];
    
    vec3 forward_no_z; glm_vec3_copy(camera.forward, forward_no_z); 
    forward_no_z[1] = 0.0f; 
    glm_vec3_normalize(forward_no_z);
    
    vec3 movement_direction = {0.0f, 0.0f, 0.0f};
    if (input_forward != 0.0f) 
    { 
        vec3 v; glm_vec3_scale(forward_no_z, input_forward, v); 
        glm_vec3_add(movement_direction, v, movement_direction); 
    }
    if (input_right != 0.0f) 
    { 
        vec3 v; glm_vec3_scale(camera.right,input_right, v); 
        glm_vec3_add(movement_direction, v, movement_direction); 
    }
    if (input_up != 0.0f) 
    { 
        vec3 v; glm_vec3_scale(WORLD_UP_VECTOR,input_up,v); 
        glm_vec3_add(movement_direction, v, movement_direction); 
    }

    // Normalize to avoid faster diagonal speed
    float move_len_sq = glm_vec3_dot(movement_direction, movement_direction);
    if (move_len_sq > input_deadzone_squared)
    {
        vec3 movement_direction_normalized, delta;
        glm_vec3_normalize_to(movement_direction, movement_direction_normalized);
        glm_vec3_scale(movement_direction_normalized, movement_speed * delta_time, delta);
        glm_vec3_add(camera.position, delta, camera.position);
    }

    vec3 camera_target;
    glm_vec3_add(camera.position, camera.forward, camera_target);
    glm_lookat(camera.position, camera_target, camera.up, camera.view_matrix);

    float aspect_ratio = (float)window_width / (float)window_height;

#define FIXED_HORIZONTAL_FOV
#ifdef FIXED_HORIZONTAL_FOV
    float fov_horizontal = glm_rad(camera.fov);
    float fov_vertical = 2.0f * SDL_atanf(SDL_tanf(fov_horizontal * 0.5f) / aspect_ratio);
#else
    float fov_vertical = glm_rad(camera.fov);
#endif

    glm_perspective
    (
        fov_vertical,
        aspect_ratio,
        camera.near_plane,
        camera.far_plane,
        camera.projection_matrix
    );

    glm_mat4_mul(camera.projection_matrix, camera.view_matrix, camera.view_projection_matrix);
}

void Camera_Log()
{
    SDL_Log
    (
        "\n"
        "Camera Position: (%.2f, %.2f, %.2f)\n"
        "Camera Forward: (%.2f, %.2f, %.2f)\n"
        "Camera Up: (%.2f, %.2f, %.2f)\n"
        "Camera Right: (%.2f, %.2f, %.2f)\n"
        "Camera Yaw: %.2f\n"
        "Camera Pitch: %.2f\n"
        "Camera FOV: %.2f\n"
        "Camera Near Plane: %.2f\n"
        "Camera Far Plane: %.2f\n",
        camera.position[0], camera.position[1], camera.position[2],
        camera.forward[0], camera.forward[1], camera.forward[2],
        camera.up[0], camera.up[1], camera.up[2],
        camera.right[0], camera.right[1], camera.right[2],
        camera.yaw, camera.pitch,
        camera.fov, camera.near_plane, camera.far_plane
    );
}