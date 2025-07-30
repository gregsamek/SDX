#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "update.h"
#include "render.h"

SDL_AppResult SDL_AppIterate(void *appstate)
{
   
    if (!Update())
    {
        return SDL_APP_FAILURE;
    }

    if (!Render())
    {
        return SDL_APP_FAILURE;
    }

    FrameRate_Update();
    
    return SDL_APP_CONTINUE;
}
