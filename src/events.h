#ifndef EVENTS_H
#define EVENTS_H

#include <SDL3/SDL.h>
#include "helper.h"

Enum (Uint8, InputState)
{
    InputState_DEFAULT,
    InputState_COUNT,
};


bool HandleEvent_InputState_DEFAULT(SDL_Event* event);

#endif // EVENTS_H