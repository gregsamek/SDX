#ifndef EVENTS_H
#define EVENTS_H

#include <SDL3/SDL.h>
#include "helper.h"

Enum (Uint8, InputState)
{
    InputState_DEBUG,
    InputState_FIRSTPERSONCONTROLLER,
    InputState_COUNT,
};


bool HandleEvent_InputState_DEBUG(SDL_Event* event);
bool HandleEvent_InputState_FIRSTPERSONCONTROLLER(SDL_Event* event);

#endif // EVENTS_H