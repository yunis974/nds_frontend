#ifndef FRONTEND_CORE_H
#define FRONTEND_CORE_H

#include <stdbool.h>
#include <libretro.h>


bool frontend_core_load(const char *core_path);
bool frontend_core_load_game(const char *rom_path);
void frontend_core_run(void);
void frontend_core_unload(void);
bool frontend_core_load_sram(void);
bool frontend_core_load_save(void);
void frontend_core_save(void);
void frontend_core_save_sram(void);

struct retro_system_av_info * get_avinfo();

#endif