#include "frontend_input.h"
#include "libretro.h"

// ---------------------------------------------------------------------------
// État interne
// ---------------------------------------------------------------------------

static SDL_Window *s_window = NULL;

static const bool *s_keys = NULL;   // état du clavier (tableau SDL, indexé par scancode)

static int16_t s_ptr_x = 0;         // position tactile au format libretro [-32767, 32767]
static int16_t s_ptr_y = 0;
static bool    s_ptr_pressed = false;

// ---------------------------------------------------------------------------
// Correspondance bouton libretro -> touche clavier
// Les cases non remplies valent 0 = SDL_SCANCODE_UNKNOWN (jamais appuyée).
// ---------------------------------------------------------------------------

static const SDL_Scancode s_keymap[] = {
    [RETRO_DEVICE_ID_JOYPAD_A]      = SDL_SCANCODE_X,
    [RETRO_DEVICE_ID_JOYPAD_B]      = SDL_SCANCODE_Z,
    [RETRO_DEVICE_ID_JOYPAD_X]      = SDL_SCANCODE_S,
    [RETRO_DEVICE_ID_JOYPAD_Y]      = SDL_SCANCODE_A,
    [RETRO_DEVICE_ID_JOYPAD_L]      = SDL_SCANCODE_Q,
    [RETRO_DEVICE_ID_JOYPAD_R]      = SDL_SCANCODE_W,
    [RETRO_DEVICE_ID_JOYPAD_START]  = SDL_SCANCODE_RETURN,
    [RETRO_DEVICE_ID_JOYPAD_SELECT] = SDL_SCANCODE_RSHIFT,
    [RETRO_DEVICE_ID_JOYPAD_UP]     = SDL_SCANCODE_UP,
    [RETRO_DEVICE_ID_JOYPAD_DOWN]   = SDL_SCANCODE_DOWN,
    [RETRO_DEVICE_ID_JOYPAD_LEFT]   = SDL_SCANCODE_LEFT,
    [RETRO_DEVICE_ID_JOYPAD_RIGHT]  = SDL_SCANCODE_RIGHT,
};

#define KEYMAP_SIZE (sizeof(s_keymap) / sizeof(s_keymap[0]))

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Convertit une coordonnée en pixels [0, size] vers l'échelle libretro [-32767, 32767]
static int16_t to_libretro_coord(float pos, int size)
{
    if (size <= 0)
        return 0;

    float norm = pos / (float)size;   // 0.0 -> 1.0
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    return (int16_t)(norm * 65534.0f - 32767.0f);
}

static bool joypad_pressed(unsigned id)
{
    if (!s_keys || id >= KEYMAP_SIZE)
        return false;

    SDL_Scancode sc = s_keymap[id];
    return sc != SDL_SCANCODE_UNKNOWN && s_keys[sc];
}

// ---------------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------------

void frontend_input_init(SDL_Window *window)
{
    s_window = window;
}

void frontend_input_poll(void)
{
    // Clavier : SDL met ce tableau à jour pendant SDL_PollEvent (dans ta boucle principale)
    s_keys = SDL_GetKeyboardState(NULL);

    // Souris -> écran tactile
    float mx, my;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    s_ptr_pressed = (buttons & SDL_BUTTON_LMASK) != 0;

    int w = 0, h = 0;
    if (s_window)
        SDL_GetWindowSize(s_window, &w, &h);

    s_ptr_x = to_libretro_coord(mx, w);
    s_ptr_y = to_libretro_coord(my, h);
}

int16_t frontend_input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;

    if (port != 0)
        return 0;   // un seul joueur pour l'instant

    switch (device) {
    case RETRO_DEVICE_JOYPAD:
        // Certains cores demandent tous les boutons d'un coup sous forme de masque
        if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
            int16_t mask = 0;
            for (unsigned i = 0; i < KEYMAP_SIZE; i++)
                if (joypad_pressed(i))
                    mask |= (int16_t)(1 << i);
            return mask;
        }
        return joypad_pressed(id) ? 1 : 0;

    case RETRO_DEVICE_POINTER:
        switch (id) {
        case RETRO_DEVICE_ID_POINTER_X:       return s_ptr_x;
        case RETRO_DEVICE_ID_POINTER_Y:       return s_ptr_y;
        case RETRO_DEVICE_ID_POINTER_PRESSED: return s_ptr_pressed ? 1 : 0;
        default:                              return 0;
        }

    default:
        return 0;
    }
}