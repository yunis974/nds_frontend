#pragma once

#include "SDL3/SDL.h"
#include <stdint.h>


bool frontend_SDL_audio_init(int freq);
void frontend_SDL_audio_deinit(void);

void audio_sample(int16_t left, int16_t right);
size_t audio_sample_batch(const int16_t *data, size_t frames);

void frontend_audio_update_rate(void);