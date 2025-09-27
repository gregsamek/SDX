#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>

#include "events.h"
#include "model.h"

bool Render_LoadRenderSettings();
bool Render_Init();
bool Render();

#endif // RENDER_H