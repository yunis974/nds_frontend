#include <SDL3/SDL.h>
#include "frontend_core.h"
#include "frontend_render.h"
#include "frontend_audio.h"

int main(int argc, char **argv)
{
    if (argc < 3) {
        SDL_Log("Usage: %s <core.so> <rom>", argv[0]);
        return 1;
    }

    // 1. Fenêtre + renderer
    if (!frontend_render_SDLinit()) {
        SDL_Log("Failed to initialize SDL.");
        return 1;
    }


    // 2. Charger le core (.so)
    if (!frontend_core_load(argv[1])) {
        SDL_Log("Failed to load core: %s", argv[1]);
        frontend_render_deinit();
        return 1;
    }

    // 3. Charger la ROM
    if (!frontend_core_load_game(argv[2])) {
        SDL_Log("Failed to load game: %s", argv[2]);
        frontend_core_unload();
        frontend_render_deinit();
        return 1;
    }

    if (!frontend_SDL_audio_init((int)get_avinfo()->timing.sample_rate))
    {
        SDL_Log("Failed to init SDL_Audio");
        frontend_core_unload();
        frontend_render_deinit();
        return 1;       
    }


    // 4. Boucle principale
    bool running = true;
    while (running) {
        // Événements (temporaire : ira dans frontend_input)
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                running = false;
        }

        frontend_core_run();        // le core émule 1 frame → appelle video_refresh
        frontend_render_present();  // on affiche la frame (attend le vsync)
    }

    // 5. Nettoyage, dans l'ordre inverse
    frontend_core_unload();
    frontend_render_deinit();
    frontend_SDL_audio_deinit();
    return 0;
}