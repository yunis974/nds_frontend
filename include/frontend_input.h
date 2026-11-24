#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

// À appeler une fois après la création de la fenêtre.
// La fenêtre sert à convertir la position de la souris pour l'écran tactile.
void frontend_input_init(SDL_Window *window);

// Callbacks libretro, à passer au core :
//   p_retro_set_input_poll(frontend_input_poll);
//   p_retro_set_input_state(frontend_input_state);
void    frontend_input_poll(void);
int16_t frontend_input_state(unsigned port, unsigned device, unsigned index, unsigned id);