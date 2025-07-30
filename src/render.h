#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>

#include "events.h"
#include "model.h"

bool Render();
void calculate_joint_matrices(Joint* joint, mat4 parent_global_transform, Uint8* joint_matrices_out, Joint* root_joint);

#endif // RENDER_H