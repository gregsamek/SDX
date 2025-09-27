#ifdef SDL3_MIXER
#include <SDL3_mixer/SDL_mixer.h>

#include "audio.h"

static Mix_Music *test_music = NULL;
static Mix_Chunk *test_sound = NULL;

bool Audio_Init()
{
    SDL_AudioSpec audio_spec = {MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, MIX_DEFAULT_FREQUENCY};

    // TODO: ability to pick audio device
    if (!Mix_OpenAudio(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec))
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", SDL_GetError());
        return false;
    }

    char music_path[MAXIMUM_URI_LENGTH];
    char sound_path[MAXIMUM_URI_LENGTH];
    SDL_snprintf(music_path, sizeof(music_path), "%saudio/test_music.wav", base_path);
    SDL_snprintf(sound_path, sizeof(sound_path), "%saudio/test_sound.wav", base_path);

    test_music = Mix_LoadMUS(music_path);

    if (test_music == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load music! SDL_mixer Error: %s\n", SDL_GetError());
        return false;
    }
    
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);

    test_sound = Mix_LoadWAV(sound_path);

    if (test_sound == NULL)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load sound effect! SDL_mixer Error: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool Audio_PlayTestSound()
{
    if (Mix_PlayChannel(-1, test_sound, 0) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to play test sound! SDL_mixer Error: %s\n", SDL_GetError());
        return false;
    }
    return true;
}
#endif // SDL3_MIXER
#include "audio.h"
bool Audio_Init() { return true; }
bool Audio_PlayTestSound() { return false; }