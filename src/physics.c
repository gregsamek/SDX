#include "physics.h"


#ifndef PHYS_INLINE
#  ifdef _MSC_VER
#    define PHYS_INLINE static __forceinline
#  else
#    define PHYS_INLINE static inline __attribute__((always_inline))
#  endif
#endif


PHYS_INLINE float vec3_len2(vec3 a){ return glm_vec3_dot(a,a); }

void Capsule_UpdatePosition(Capsule* capsule, vec3 newPosition)
{
    glm_vec3_copy(newPosition, capsule->position);
    capsule->topSphereCenter[0] = capsule->position[0];
    capsule->topSphereCenter[1] = capsule->position[1] + (capsule->height - 2.0f * capsule->radius);
    capsule->topSphereCenter[2] = capsule->position[2];
    capsule->aabb[0][0] = capsule->position[0] - capsule->radius;
    capsule->aabb[0][1] = capsule->position[1] - capsule->radius;
    capsule->aabb[0][2] = capsule->position[2] - capsule->radius;
    capsule->aabb[1][0] = capsule->position[0] + capsule->radius;
    capsule->aabb[1][1] = capsule->position[1] + capsule->height;
    capsule->aabb[1][2] = capsule->position[2] + capsule->radius;
}

void AABBFromTri(Tri tri, vec3 aabb[2])
{
    aabb[0][0] = SDL_min(tri.a[0], SDL_min(tri.b[0], tri.c[0]));
    aabb[0][1] = SDL_min(tri.a[1], SDL_min(tri.b[1], tri.c[1]));
    aabb[0][2] = SDL_min(tri.a[2], SDL_min(tri.b[2], tri.c[2]));

    aabb[1][0] = SDL_max(tri.a[0], SDL_max(tri.b[0], tri.c[0]));
    aabb[1][1] = SDL_max(tri.a[1], SDL_max(tri.b[1], tri.c[1]));
    aabb[1][2] = SDL_max(tri.a[2], SDL_max(tri.b[2], tri.c[2]));
}

void ClosestPointOnTriangle(vec3 p, vec3 a, vec3 b, vec3 c, vec3 out)
{
    // Check if P in vertex region outside A
    vec3 ab;
    glm_vec3_sub(b, a, ab);
    vec3 ac;
    glm_vec3_sub(c, a, ac);
    vec3 ap;
    glm_vec3_sub(p, a, ap);

    float d1 = glm_vec3_dot(ab, ap);
    float d2 = glm_vec3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) // barycentric coordinates (1,0,0)
    {
        glm_vec3_copy(a, out);
        return;
    }

    // Check if P in vertex region outside B
    vec3 bp;
    glm_vec3_sub(p,b, bp);
    float d3 = glm_vec3_dot(ab, bp);
    float d4 = glm_vec3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) // barycentric coordinates (0,1,0)
    {
        glm_vec3_copy(b, out);
        return;
    }

    // Check if P in edge region of AB, if so return projection of P onto AB
    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) 
    {
        float v = d1 / (d1 - d3);
        vec3 temp;
        glm_vec3_scale(ab, v, temp);
        glm_vec3_add(a, temp, out); // barycentric coordinates (1-v,v,0)
        return;
    }

    // Check if P in vertex region outside C
    vec3 cp;
    glm_vec3_sub(p, c, cp);
    float d5 = glm_vec3_dot(ab, cp);
    float d6 = glm_vec3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) // barycentric coordinates (0,0,1)
    {
        glm_vec3_copy(c, out);
        return;
    }

    // Check if P in edge region of AC, if so return projection of P onto AC
    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) 
    {
        float w = d2 / (d2 - d6);
        vec3 temp;
        glm_vec3_scale(ac, w, temp);
        glm_vec3_add(a, temp, out); // barycentric coordinates (1-w,0,w)
        return;
    }

    // Check if P in edge region of BC, if so return projection of P onto BC
    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) 
    {
        vec3 bc;
        glm_vec3_sub(c,b, bc);
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        vec3 temp;
        glm_vec3_scale(bc, w, temp);
        glm_vec3_add(b, temp, out); // barycentric coordinates (0,1-w,w)
        return;
    }

    // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    vec3 temp1, temp2;
    glm_vec3_scale(ab, v, temp1);
    glm_vec3_scale(ac, w, temp2);
    glm_vec3_add(temp1, temp2, temp1);
    glm_vec3_add(a, temp1, out); // = u*a + v*b + w*c, u = va * denom = 1.0f-v-w
    return;
}

void ClosestPointsSegmentSegment
(
    vec3 p1, vec3 q1, vec3 p2, vec3 q2,
    float* sOut, float* tOut, 
    vec3 c1Out, vec3 c2Out
){
    vec3 d1;
    glm_vec3_sub(q1, p1, d1);
    vec3 d2;
    glm_vec3_sub(q2, p2, d2);
    vec3 r;
    glm_vec3_sub(p1, p2, r);

    float a = glm_vec3_dot(d1,d1);
    float e = glm_vec3_dot(d2,d2);
    float f = glm_vec3_dot(d2,r);

    float s, t;

    if (a <= 1e-8f && e <= 1e-8f) {
        s = t = 0.0f;
        glm_vec3_copy(p1, c1Out);
        glm_vec3_copy(p2, c2Out);
    } else if (a <= 1e-8f) {
        s = 0.0f;
        t = f / e;
        t = SDL_max(0.0f, SDL_min(1.0f, t));
    } else {
        float c = glm_vec3_dot(d1,r);
        if (e <= 1e-8f) {
            t = 0.0f;
            s = SDL_max(0.0f, SDL_min(1.0f, -c / a));
        } else {
            float b = glm_vec3_dot(d1,d2);
            float denom = a*e - b*b;
            if (denom != 0.0f) s = (b*f - c*e) / denom;
            else s = 0.0f;
            s = SDL_max(0.0f, SDL_min(1.0f, s));
            t = (b*s + f) / e;

            if (t < 0.0f) {
                t = 0.0f;
                s = SDL_max(0.0f, SDL_min(1.0f, -c / a));
            } else if (t > 1.0f) {
                t = 1.0f;
                s = SDL_max(0.0f, SDL_min(1.0f, (b - c) / a));
            }
        }
    }

    vec3 temp;
    glm_vec3_scale(d1, s, temp);
    glm_vec3_add(p1, temp, c1Out);
    glm_vec3_scale(d2, t, temp);
    glm_vec3_add(p2, temp, c2Out);

    if (sOut) *sOut = s;
    if (tOut) *tOut = t;
}

Penetration CapsuleTrianglePenetration(Capsule* capsule, Tri* tri, vec3 n) 
{
    vec3 a,b,c;
    glm_vec3_copy(tri->a, a);
    glm_vec3_copy(tri->b, b);
    glm_vec3_copy(tri->c, c);

    // Candidate closest pair
    vec3 bestSeg;
    glm_vec3_copy(capsule->bottomSphereCenter, bestSeg);
    vec3 bestTri;
    ClosestPointOnTriangle(capsule->bottomSphereCenter, a,b,c, bestTri);
    vec3 temp;
    glm_vec3_sub(bestSeg, bestTri, temp);
    float bestD2 = vec3_len2(temp);

    // Check other endpoint
    {
        vec3 tpt;
        ClosestPointOnTriangle(capsule->topSphereCenter, a,b,c, tpt);
        glm_vec3_sub(capsule->topSphereCenter, tpt, temp);
        float d2 = vec3_len2(temp);
        if (d2 < bestD2)
        {
            bestD2 = d2; 
            glm_vec3_copy(capsule->topSphereCenter, bestSeg); 
            glm_vec3_copy(tpt, bestTri); 
        }
    }

    // Check segment vs triangle edges
    vec3 edges[3][2]; // { {a,b}, {b,c}, {c,a} };
    glm_vec3_copy(a, edges[0][0]);
    glm_vec3_copy(b, edges[0][1]);
    glm_vec3_copy(b, edges[1][0]);
    glm_vec3_copy(c, edges[1][1]);
    glm_vec3_copy(c, edges[2][0]);
    glm_vec3_copy(a, edges[2][1]);

    for (int i = 0; i < 3; ++i) 
    {
        vec3 cSeg, cEdge;
        ClosestPointsSegmentSegment(capsule->bottomSphereCenter, capsule->topSphereCenter, edges[i][0], edges[i][1], 0,0, cSeg, cEdge);
        glm_vec3_sub(cSeg, cEdge, temp);
        float d2 = vec3_len2(temp);
        if (d2 < bestD2)
        {
            bestD2 = d2; 
            glm_vec3_copy(cSeg, bestSeg); 
            glm_vec3_copy(cEdge, bestTri); 
        }
    }

    // Optional: check if segment crosses triangle face region (helps when passing through the face)
    // Project segment onto plane and see if intersection point lies inside triangle.
    {
        vec3 dseg;
        glm_vec3_sub(capsule->topSphereCenter, capsule->bottomSphereCenter, dseg);
        float denom = glm_vec3_dot(n, dseg);
        if (fabsf(denom) > 1e-8f) 
        {
            glm_vec3_sub(a, capsule->bottomSphereCenter, temp);
            float t = glm_vec3_dot(n, temp) / denom;
            if (t >= 0.0f && t <= 1.0f) 
            {
                vec3 p;
                glm_vec3_scale(dseg, t, temp);
                glm_vec3_add(capsule->bottomSphereCenter, temp, p);
                vec3 cp;
                ClosestPointOnTriangle(p, a, b, c, cp);
                // If p is in the triangle, cp == p (within float noise).
                glm_vec3_sub(p, cp, temp);
                float d2 = vec3_len2(temp);
                if (d2 < bestD2)
                { 
                    bestD2 = d2; 
                    glm_vec3_copy(p, bestSeg); 
                    glm_vec3_copy(cp, bestTri); 
                }
            }
        }
    }

    float r = capsule->radius;
    if (bestD2 >= r*r) return (Penetration){0};

    float dist = sqrtf(SDL_max(bestD2, 0.0f));
    float depth = r - dist;

    vec3 pushDir;
    glm_vec3_sub(bestSeg, bestTri, pushDir);

    vec3 normal;
    if (vec3_len2(pushDir) > 1e-12f) 
    {
        // Best case: exact push-out direction
        glm_vec3_normalize_to(pushDir, normal);
    } 
    else 
    {
        // Degenerate: fall back to triangle normal, but orient it toward the capsule
        glm_vec3_copy(n, normal); // computed from cross product (may be flipped by winding)

        vec3 mid;
        glm_vec3_add(capsule->bottomSphereCenter, capsule->topSphereCenter, mid);
        glm_vec3_scale(mid, 0.5f, mid);

        vec3 toCapsule;
        glm_vec3_sub(mid, bestTri, toCapsule);

        if (glm_vec3_dot(normal, toCapsule) < 0.0f) 
        {
            glm_vec3_scale(normal, -1.0f, normal);
        }
    }

    Penetration out =
    {
        .hit = 1,
        .depth = depth
    };

    glm_vec3_copy(normal, out.normal);
    return out;
}

void MoveAndSlide(Capsule* capsule, Collider Array colliders, float dt) 
{
    const float skin = 0.001f;       // small separation to avoid jitter
    const int maxIters = 4;

    const float maxSlopeDeg = 50.0f;
    const float maxSlopeCos = SDL_cosf(maxSlopeDeg * (3.14159265f / 180.0f));

    vec3 displacement = { capsule->velocity[0] * dt, capsule->velocity[1] * dt, capsule->velocity[2] * dt };

    bool grounded = false;
    vec3 groundNormal = {0,0,0};

    for (int iter = 0; iter < maxIters; ++iter) 
    {
        vec3 newPosition;
        glm_vec3_add(capsule->position, displacement, newPosition);
        Capsule_UpdatePosition(capsule, newPosition);

        int anyPush = 0;

        // TODO replace loop with broadphase query of triangles e.g. spatial grid
        for (int i = 0; i < Array_Len(colliders); ++i) 
        {
            if (!glm_aabb_aabb(capsule->aabb, colliders[i].aabb)) continue;
            
            Penetration penetration = CapsuleTrianglePenetration(capsule, &colliders[i].tri, colliders[i].normal);
            
            if (!penetration.hit) continue;

            anyPush = 1;

            // TODO this doesn't check how high the collider is relative to the capsule base
            // if the top of the capsule collides with a ledge, would that count as ground?
            if (penetration.normal[1] >= maxSlopeCos && glm_vec3_dot(capsule->velocity, penetration.normal) < 0.0f)
            {
                grounded = true;
                glm_vec3_copy(penetration.normal, groundNormal); // TODO keep the steepest normal?
            }

            // push out (depenetrate)
            float push = penetration.depth + skin;
            vec3 pushVec;
            glm_vec3_scale(penetration.normal, push, pushVec);
            glm_vec3_add(capsule->position, pushVec, newPosition);
            Capsule_UpdatePosition(capsule, newPosition);

            // slide: remove motion/velocity into the contact
            float velocity_normal = glm_vec3_dot(capsule->velocity, penetration.normal);
            
            if (velocity_normal < 0.0f)
            {
                vec3 temp;
                glm_vec3_scale(penetration.normal, velocity_normal, temp);
                glm_vec3_sub(capsule->velocity, temp, capsule->velocity);
            }

            float displacement_normal = glm_vec3_dot(displacement, penetration.normal);
            
            if (displacement_normal < 0.0f) 
            {
                vec3 temp;
                glm_vec3_scale(penetration.normal, displacement_normal, temp);
                glm_vec3_sub(displacement, temp, displacement);
            }
        }

        if (!anyPush) break;

        // optional: shrink remaining disp each iteration to avoid micro-oscillation
        glm_vec3_scale(displacement, 0.5f, displacement);
    }

    capsule->grounded = grounded;
    glm_vec3_copy(groundNormal, capsule->groundNormal);

    if (grounded && capsule->velocity[1] < 0.0f)
        capsule->velocity[1] = 0.0f;

    // TODO down cast + snap if found to be necessary during playtesting (jitter)
}