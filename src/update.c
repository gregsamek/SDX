#include <SDL3/SDL.h>

#include "update.h"
#include "globals.h"

bool Update()
{
    current_ticks = SDL_GetPerformanceCounter();
    delta_time = (float)((double)(current_ticks - last_ticks) / performance_frequency);
    last_ticks = current_ticks;
    
    if (delta_time > MAXIMUM_DELTA_TIME)
    {
        delta_time = MAXIMUM_DELTA_TIME;
    }

    if (input_state == InputState_DEFAULT)
    {
        Camera_Update();
    }

    return true;
}

void Update_FrameRate(void)
{
    #define FRAME_TIME_ARRAY_SIZE 512
    static Uint64 previous_frame_end_ticks = 0;
    static float frame_times[FRAME_TIME_ARRAY_SIZE] = {0};
    static Uint16 frame_time_index = 0;
    
    current_ticks = SDL_GetPerformanceCounter();

    double frame_time = (double)(current_ticks - previous_frame_end_ticks) / performance_frequency;
    frame_times[frame_time_index] = frame_time;
    frame_time_index = (frame_time_index + 1) & (FRAME_TIME_ARRAY_SIZE - 1);

    if (frame_time_index == 0)
    {
        // Calculate average frame time
        double total_frame_time = 0.0;
        for (int i = 0; i < FRAME_TIME_ARRAY_SIZE; i++)
        {
            total_frame_time += frame_times[i];
        }
        double average_frame_time = total_frame_time / FRAME_TIME_ARRAY_SIZE;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%f avg fps over last %d frames", 1.0 / average_frame_time, FRAME_TIME_ARRAY_SIZE);
    }

    if (manage_frame_rate_manually && (frame_time < minimum_frame_time)) 
    {
        SDL_DelayPrecise((minimum_frame_time - frame_time) * 1000000000.0);
    }

    previous_frame_end_ticks = SDL_GetPerformanceCounter();
}