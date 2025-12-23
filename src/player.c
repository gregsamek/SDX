#include "player.h"
#include "globals.h"

bool Player_Init(Player* player, vec3 startPosition, float height, float radius)
{
    if (!player || !startPosition)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Player_Init: invalid parameters");
        return false;
    }

    player->capsule.height = height;
    player->capsule.radius = radius;
    Capsule_UpdatePosition(&player->capsule, startPosition);
    glm_vec3_zero(player->capsule.velocity);
    player->capsule.grounded = false;
    glm_vec3_zero(player->capsule.groundNormal);
    
    player->eyeHeightOffset = height * 0.9f; // eyes at 90% of capsule height
    
    player->camera = (Camera)
    {
        .position = {0.0f, 0.0f, 0.0f},
        .forward = {0},
        .up = {0},
        .right = {0},
        .yaw = 222.0f,
        .pitch = 0.0f,
        .fov = 70.0f,
        .near_plane = 0.1f,
        .far_plane = 200.0f,
        .view_projection_matrix = {0}
    };
    activeCamera = &player->camera;
    
    player->movementSpeed = 10.0f; // meters per second

    return true;
}

void Player_IntendedVelocity(Player* player)
{
    glm_vec3_zero(player->capsule.velocity);

    float input_forward = (int)keyboard_state[SDL_SCANCODE_W] - (int)keyboard_state[SDL_SCANCODE_S];
    float input_right   = (int)keyboard_state[SDL_SCANCODE_D] - (int)keyboard_state[SDL_SCANCODE_A];
    
    vec3 forward_no_z;
    glm_vec3_copy(player->camera.forward, forward_no_z); 
    forward_no_z[1] = 0.0f;
    
    // Only normalize if the vector isn't zero length to avoid NaN
    if (glm_vec3_norm2(forward_no_z) > 1e-6f) 
        glm_vec3_normalize(forward_no_z);
    else 
        glm_vec3_zero(forward_no_z); // Fallback if looking straight up/down (though pitch clamp helps, this is safer)
 
    vec3 movement_direction = {0.0f, 0.0f, 0.0f};
    if (input_forward != 0.0f) 
    { 
        vec3 v; glm_vec3_scale(forward_no_z, input_forward, v); 
        glm_vec3_add(movement_direction, v, movement_direction); 
    }
    if (input_right != 0.0f) 
    { 
        vec3 v; glm_vec3_scale(player->camera.right,input_right, v); 
        glm_vec3_add(movement_direction, v, movement_direction); 
    }

    // Normalize to avoid faster diagonal speed
    float move_len_sq = glm_vec3_dot(movement_direction, movement_direction);
    if (move_len_sq > input_deadzone_squared)
    {
        vec3 movementDirectionNormalized;
        glm_vec3_normalize_to(movement_direction, movementDirectionNormalized);
        glm_vec3_scale(movementDirectionNormalized, player->movementSpeed, player->capsule.velocity);
    }

    player->capsule.velocity[1] = -10.0f;

    // if (player->capsule.grounded)
    // {
    //     player->capsule.velocity[1] = 0.0f;

    //     // project velocity onto ground plane
    //     float velocity_into_ground = glm_vec3_dot(player->capsule.velocity, player->capsule.groundNormal);
    //     vec3 v;
    //     glm_vec3_scale(player->capsule.groundNormal, velocity_into_ground, v);
    //     glm_vec3_sub(player->capsule.velocity, v, player->capsule.velocity);

    //     // with this normalization, xz movement speed is always constant, even on slopes (may or may not be desired)
    //     glm_vec3_normalize_to(player->capsule.velocity, player->capsule.velocity);
    //     glm_vec3_scale(player->capsule.velocity, player->movementSpeed, player->capsule.velocity);
    // }
    // else
    // {
    //     player->capsule.velocity[1] = -10.0f;
    // }
}

void Player_Print(Player* player)
{
    if (!player) return;

    SDL_Log("Player Capsule Position: (%f, %f, %f)", player->capsule.position[0], player->capsule.position[1], player->capsule.position[2]);
    SDL_Log("Player Capsule Velocity: (%f, %f, %f)", player->capsule.velocity[0], player->capsule.velocity[1], player->capsule.velocity[2]);
    SDL_Log("Player Capsule Grounded: %s", player->capsule.grounded ? "true" : "false");
}