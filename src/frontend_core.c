#include <SDL3/SDL.h>       // SDL_Log
#include <dlfcn.h>          // dlopen, dlsym, dlclose
#include <stdio.h>          // fopen, fread, printf
#include <stdlib.h>         // malloc, free
#include <stdarg.h>         // va_list (pour le log)
#include "frontend_core.h"
#include "frontend_render.h"
#include "frontend_audio.h"
#include "frontend_input.h"
#include <limits.h>

static void *core_handle = NULL;
static void *rom_data = NULL;
static char base_dir[PATH_MAX];

static bool game_loaded = false;
static bool core_loaded = false;

static struct retro_system_av_info av_info;

static void (*core_set_environment)(retro_environment_t);
static void (*core_set_video_refresh)(retro_video_refresh_t);
static void (*core_set_audio_sample)(retro_audio_sample_t);
static void (*core_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void (*core_set_input_poll)(retro_input_poll_t);
static void (*core_set_input_state)(retro_input_state_t);
static void (*core_init)(void);
static void (*core_deinit)(void);
static void (*core_get_system_info)(struct retro_system_info *);
static void (*core_get_system_av_info)(struct retro_system_av_info *);
static bool (*core_load_game)(const struct retro_game_info *);
static void (*core_unload_game)(void);
static void (*core_run)(void);


struct symbol_entry 
{
    const char *name; 
    void **ptr;
};

static const struct symbol_entry retro_symbols[] = {
    {"retro_init", (void **)&core_init},
    {"retro_deinit", (void **)&core_deinit},
    {"retro_get_system_info", (void **)&core_get_system_info},
    {"retro_get_system_av_info", (void **)&core_get_system_av_info},
    {"retro_load_game", (void **)&core_load_game},
    {"retro_unload_game", (void **)&core_unload_game},
    {"retro_run", (void **)&core_run},
    {"retro_set_environment", (void **)&core_set_environment},
    {"retro_set_video_refresh", (void **)&core_set_video_refresh},
    {"retro_set_audio_sample", (void **)&core_set_audio_sample},
    {"retro_set_audio_sample_batch", (void **)&core_set_audio_sample_batch},
    {"retro_set_input_poll", (void **)&core_set_input_poll},
    {"retro_set_input_state", (void **)&core_set_input_state},
};



//callback pour le core implementer dans libretro.h



static void core_log(enum retro_log_level level, const char *fmt, ...)
{
    static const char *names[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (level <= RETRO_LOG_ERROR)
        fprintf(stderr, "[core %s] ", names[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}



static bool env_cb(unsigned cmd, void *data)
{
    switch(cmd)
    {
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            return frontend_render_set_pixel_format(*(enum retro_pixel_format *)data);

        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            *(bool *)data = true;
            return true;

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            *(const char **)data = base_dir;
            return true;

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            *(const char **)data = base_dir;
            return true;
        
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            ((struct retro_log_callback *)data)->log = core_log;
            return true;
            
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            *(bool *)data = false;
            return true;
        default:
            SDL_Log("Unknown environment command: %u", cmd);
            return false;
    }

}







bool frontend_core_load(const char *core_path)
{
    if (!realpath(".", base_dir)) {
        SDL_Log("realpath failed");
        return false;
    }
    SDL_Log("Base dir: %s", base_dir);

    core_handle = dlopen(core_path, RTLD_LAZY);
    if (!core_handle)
    {
        SDL_Log("Failed to load core: %s", dlerror());
        return false;
    }

    for (size_t i = 0; i < sizeof(retro_symbols) / sizeof(retro_symbols[0]); i++)
    {
        *retro_symbols[i].ptr = dlsym(core_handle, retro_symbols[i].name);
        if (!*retro_symbols[i].ptr)
        {
            SDL_Log("Failed to load symbol %s: %s", retro_symbols[i].name, dlerror());
            dlclose(core_handle);
            core_handle = NULL;
            return false;
        }
    }

    struct retro_system_info info = {0};
    core_get_system_info(&info);
    SDL_Log("Core: %s %s", info.library_name, info.library_version);

    core_set_environment(env_cb);
    core_init();
    core_set_video_refresh(frontend_render_video_refresh);
    core_set_audio_sample(audio_sample);
    core_set_audio_sample_batch(audio_sample_batch);
    core_set_input_poll(frontend_input_poll);
    core_set_input_state(frontend_input_state);

    core_loaded = true;
    return true;
}   

bool frontend_core_load_game(const char *rom_path)
{
    if (!core_loaded)
    {
        SDL_Log("Core not loaded.");
        return false;
    }

    FILE *file = fopen(rom_path, "rb");
    if (!file)
    {
        SDL_Log("Failed to open ROM: %s", rom_path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    rom_data = malloc(size);
    if (!rom_data)
    {
        SDL_Log("Failed to allocate memory for ROM.");
        fclose(file);
        return false;
    }

    fread(rom_data, 1, size, file);
    fclose(file);

    struct retro_game_info game_info = {0};
    game_info.path = rom_path;
    game_info.data = rom_data;
    game_info.size = size;

    if (!core_load_game(&game_info))
    {
        SDL_Log("Failed to load game: %s", rom_path);
        free(rom_data);
        rom_data = NULL;
        return false;
    }

    core_get_system_av_info(&av_info);    

    SDL_Log("Video: %ux%u (max %ux%u), %.2f fps",
            av_info.geometry.base_width, av_info.geometry.base_height,
            av_info.geometry.max_width,  av_info.geometry.max_height,
            av_info.timing.fps);

    if (!frontend_render_set_geometry(&av_info.geometry))
    {
        SDL_Log("Failed to set geometry.");
        core_unload_game();
        free(rom_data);
        rom_data = NULL;
        return false;
    }

    game_loaded = true;
    return true;

}

void frontend_core_run(void)
{
    if (game_loaded)
        core_run();
}

void frontend_core_unload(void)
{
    if (game_loaded) {
        core_unload_game();
        game_loaded = false;
    }
    free(rom_data);           // free(NULL) ne fait rien, pas besoin de vérifier
    rom_data = NULL;

    if (core_loaded) {
        core_deinit();
        core_loaded = false;
    }
    if (core_handle) {
        dlclose(core_handle); // décharge le .so : après ça, tous les pointeurs de fonction sont invalides
        core_handle = NULL;
    }
}


const struct retro_system_av_info *frontend_core_get_av_info(void)
{
    return &av_info;
}

struct retro_system_av_info * get_avinfo()
{
    return &av_info;
}