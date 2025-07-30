#ifndef EVENTS_H
#define EVENTS_H

#include <SDL3/SDL.h>

typedef Uint8 InputState; enum
{
    InputState_DEFAULT,
    InputState_COUNT,
};

bool HandleWindowResize();
bool HandleEvent_InputState_DEFAULT(SDL_Event* event);

#endif // EVENTS_H