#ifndef PLAYER_H
#define PLAYER_H

#include "physics.h"
#include "camera.h"

Struct (Player)
{
    Capsule capsule;
    Camera camera;
    float eyeHeightOffset; // height of eyes from bottom of capsule; meters
    float movementSpeed;   // meters per second
};

bool Player_Init(Player* player, vec3 startPosition, float height, float radius);
void Player_IntendedVelocity(Player* player);
void Player_Print(Player* player);

#endif // PLAYER_H