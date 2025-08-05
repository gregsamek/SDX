#include "camera.h"
#include "globals.h"

static float camera_pitch_adjust_speed = 80.0f;

void Camera_Update()
{
    vec3 move_direction = {0.0f, 0.0f, 0.0f};
    if (keyboard_state[SDL_SCANCODE_W])
    {
        move_direction[2] += 1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_S])
    {
        move_direction[2] -= 1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_A])
    {
        move_direction[0] -= 1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_D])
    {
        move_direction[0] += 1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_Q])
    {
        camera.pitch += 1.0f * camera_pitch_adjust_speed * delta_time; // Rotate up
    }
    if (keyboard_state[SDL_SCANCODE_E])
    {
        camera.pitch -= 1.0f * camera_pitch_adjust_speed * delta_time; // Rotate down
    }
    if (keyboard_state[SDL_SCANCODE_SPACE])
    {
        move_direction[1] += 1.0f; // Move along world up
    }
    if (keyboard_state[SDL_SCANCODE_LCTRL] || keyboard_state[SDL_SCANCODE_RCTRL])
    {
        move_direction[1] -= 1.0f; // Move along world down
    }

    // Clamp pitch
    if (camera.pitch > 89.0f)
        camera.pitch = 89.0f;
    if (camera.pitch < -89.0f)
        camera.pitch = -89.0f;

    // Normalize move_direction if it's not zero to prevent faster diagonal movement
    float move_len_sq = glm_vec3_dot(move_direction, move_direction);
    if (move_len_sq > input_deadzone_squared)
    {
        vec3 normMoveDir;
        glm_vec3_normalize_to(move_direction, normMoveDir);
        camera.position[0] += normMoveDir[0] * movement_speed * delta_time;
        camera.position[1] += normMoveDir[1] * movement_speed * delta_time;
        camera.position[2] += normMoveDir[2] * movement_speed * delta_time;
    }
    
    vec3 forward;
    forward[0] = SDL_cosf(glm_rad(camera.yaw)) * SDL_cosf(glm_rad(camera.pitch));
    forward[1] = SDL_sinf(glm_rad(camera.pitch));
    forward[2] = SDL_sinf(glm_rad(camera.yaw)) * SDL_cosf(glm_rad(camera.pitch));
    glm_vec3_normalize_to(forward, camera.forward);
    
    glm_vec3_crossn(camera.forward, WORLD_UP_VECTOR, camera.right);
    glm_vec3_crossn(camera.right, camera.forward, camera.up);

    vec3 camera_target = { camera.position[0] + camera.forward[0], camera.position[1] + camera.forward[1], camera.position[2] + camera.forward[2] };
    mat4 view_matrix;
    glm_lookat(camera.position, camera_target, camera.up, view_matrix);

    mat4 projection_matrix;
    glm_perspective
    (
        glm_rad(camera.fov),
        (float)window_width / (float)window_height,
        camera.near_plane,
        camera.far_plane,
        projection_matrix
    );

    glm_mat4_mul(projection_matrix, view_matrix, camera.view_projection_matrix);
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