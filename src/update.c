#include <SDL3/SDL.h>

#include "update.h"
#include "globals.h"

bool Update()
{
    current_ticks = SDL_GetPerformanceCounter();
    delta_time = (float)((double)(current_ticks - last_ticks) / performance_frequency);
    last_ticks = current_ticks;
    
    if (delta_time > MAXIMUM_DELTA_TIME) delta_time = MAXIMUM_DELTA_TIME;

    Camera_UpdateDirection(activeCamera);

    switch (input_state)
    {
        case InputState_DEBUG:
            Camera_MoveNoClip(&camera_noClip);
            break;
        case InputState_FIRSTPERSONCONTROLLER:
            Player_IntendedVelocity(&player);
            MoveAndSlide(&player.capsule, colliders, delta_time);
            glm_vec3_copy(player.capsule.position, player.camera.position);
            player.camera.position[1] += player.eyeHeightOffset;
            CheckTriggers(&player.capsule, triggers);
            break;
        default:
            break;
    }

    Camera_UpdateMatrices(activeCamera);

    return true;
}

void Update_FrameRate(void)
{
    #define FRAME_TIME_ARRAY_SIZE 128
    static Uint64 previous_frame_end_ticks = 0;
    static float frame_times[FRAME_TIME_ARRAY_SIZE] = {0};
    static Uint16 frame_time_index = 0;
    
    current_ticks = SDL_GetPerformanceCounter();

    double frame_time = (double)(current_ticks - previous_frame_end_ticks) / performance_frequency;
    frame_times[frame_time_index] = frame_time;
    frame_time_index = (frame_time_index + 1) & (FRAME_TIME_ARRAY_SIZE - 1);

    if (frame_time_index == 0)
    {
        double total_frame_time = 0.0;
        for (int i = 0; i < FRAME_TIME_ARRAY_SIZE; i++)
        {
            total_frame_time += frame_times[i];
        }
        average_frame_rate = 1.0 / (total_frame_time / FRAME_TIME_ARRAY_SIZE);
    }

    if ((swapchain_present_mode == SDL_GPU_PRESENTMODE_IMMEDIATE) && (frame_time < minimum_frame_time)) 
    {
        SDL_DelayPrecise((minimum_frame_time - frame_time) * 1000000000.0);
    }

    previous_frame_end_ticks = SDL_GetPerformanceCounter();
}