#ifndef PHYSICS_H
#define PHYSICS_H

#include "helper.h"
#include "array.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED
#include "../external/cglm/cglm.h"

// Struct (Vec3)
// {
//     union
//     {
//         float vec3[3];
//         struct 
//         {
//             float x;
//             float y;
//             float z;
//         };
//     };
// };

Struct (Tri) 
{
    vec3 a;
    vec3 b;
    vec3 c;
};

Struct (Collider) 
{
    Tri tri;
    vec3 aabb[2];
    vec3 normal; // precomputed face normal
};

typedef bool (*Trigger_Callback)(void);

// TODO exit callback requires state, extra code in trigger loop.
Struct (Trigger)
{
    vec3 aabb[2];
    Trigger_Callback callback_enter;
    // Trigger_Callback callback_exit;
};

Struct (Capsule) 
{
    union
    {
        vec3 position;
        vec3 bottomSphereCenter;
    };
    vec3 topSphereCenter;
    vec3 velocity;       // velocity
    vec3 groundNormal;   // normal of ground we're standing on
    vec3 aabb[2];        // precomputed AABB for broadphase
    float radius;        // capsule radius
    float height;        // capsule full height (>= 2*radius)
    bool grounded;       // is player on ground
};

Struct (Penetration) 
{
    vec3 normal;   // unit normal pointing OUT of triangle (push direction)
    float depth;   // penetration depth (> 0)
    bool hit;
};

void AABBFromTri(Tri tri, vec3 aabb[2]);

void ClosestPointOnTriangle(vec3 p, vec3 a, vec3 b, vec3 c, vec3 out);

void ClosestPointsSegmentSegment
(
    vec3 p1, vec3 q1, vec3 p2, vec3 q2,
    float* sOut, float* tOut,
    vec3 c1Out, vec3 c2Out
);

Penetration CapsuleTrianglePenetration(Capsule* player, Tri* tri, vec3 normal);
void Capsule_UpdatePosition(Capsule* capsule, vec3 newPosition);
void MoveAndSlide(Capsule* capsule, Collider Array colliders, float dt);

void CheckTriggers(Capsule* capsule, Trigger Array triggers);
bool Trigger_DummyCallback(void);

#endif // PHYSICS_H