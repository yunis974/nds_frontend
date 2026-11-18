#ifndef frontend_render_h
#define frontend_render_h

#include <libretro.h>
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>

// Function to initialize SDL for rendering
bool frontend_render_SDLinit(void);

//getter to get window and renderer outside frontend_render.c
SDL_Window* frontend_render_get_window(void);
SDL_Renderer* frontend_render_get_renderer(void);


//libretro function
bool frontend_render_set_pixel_format(enum retro_pixel_format fmt);
bool frontend_render_set_geometry(const struct retro_game_geometry *geom);
void frontend_render_video_refresh(const void *data, unsigned int width, unsigned int height, size_t pitch);
void frontend_render_present(void);
void frontend_render_deinit(void);


#endif