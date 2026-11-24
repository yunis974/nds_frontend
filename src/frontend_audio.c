#include "frontend_audio.h"

#define TARGET_S   0.06  
#define MAX_DELTA  0.005 

static SDL_AudioStream *stream = NULL;

bool frontend_SDL_audio_init(int freq)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("SDL audio init: %s", SDL_GetError());
        return false;
    }

    SDL_AudioSpec spec = {
        .format   = SDL_AUDIO_S16,
        .channels = 2,
        .freq     = freq,
    };

    stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("SDL open audio: %s", SDL_GetError());
        return false;
    }

    SDL_ResumeAudioStreamDevice(stream);
    return true;
}

void frontend_SDL_audio_deinit(void)
{
    SDL_DestroyAudioStream(stream);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void audio_sample(int16_t left, int16_t right)
{
    int16_t frame[2] = { left, right };
    audio_sample_batch(frame, 1); 
}

size_t audio_sample_batch(const int16_t *data, size_t frames)
{
    if (!stream)
    {
        return frames;
    }

    SDL_PutAudioStreamData(stream, data, (int)(frames * 2 * sizeof(int16_t)));
    return frames;
}


