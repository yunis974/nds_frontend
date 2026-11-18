#include "frontend_render.h"



static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_XRGB1555;
static unsigned int frame_width = 0;
static unsigned int frame_height = 0;
static float aspect_ratio = 0.0f;

//init the sdl window and renderer context, it check for errors and log them if any
bool frontend_render_SDLinit(void) {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("NDS Frontend",1280,720,SDL_WINDOW_FULLSCREEN);
    if(!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if(!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        return false;
    }
    
    SDL_SetRenderVSync(renderer, 1);

    return true;
}


SDL_Window* frontend_render_get_window(void) {
    if (!window) {
        SDL_Log("Window has not been initialized.");
        return NULL;
    }

    return window;
}

SDL_Renderer* frontend_render_get_renderer(void) {
    if (!renderer) {
        SDL_Log("Renderer has not been initialized.");
        return NULL;
    }

    return renderer;
}


/*cette fonction permet de convertir le format de pixel envoyer par le core
dans le bon format correspondant en sdl*/
bool frontend_render_set_pixel_format(enum retro_pixel_format fmt) 
{
    switch (fmt) 
    {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            pixel_format = SDL_PIXELFORMAT_XRGB8888;
            break;
        
        case RETRO_PIXEL_FORMAT_RGB565:
            pixel_format = SDL_PIXELFORMAT_RGB565;
            break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
            pixel_format = SDL_PIXELFORMAT_XRGB1555;
            break;
        
        default:
            SDL_Log("Unknown pixel format: %d", fmt);
            return false;
    }

    return true;
}


/*Cette fonction recupere la sortie video generer par le core et creer 
uen texture sdl avec les bonne dimensions*/
bool frontend_render_set_geometry(const struct retro_game_geometry *geom)
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }

    texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, (int)geom->max_width, (int)geom->max_height);
    if (!texture)
    {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    frame_width = geom->base_width;
    frame_height = geom->base_height;
    aspect_ratio = geom->aspect_ratio;

    return true;

}



void frontend_render_video_refresh(const void *data, unsigned int width, unsigned int height, size_t pitch)
{
    if (!data)
    {
        return;
    }

    if (!texture )
    {
        SDL_Log("Texture is NULL.");
        return;
    }

    SDL_Rect rect = {0, 0, (int)width, (int)height};
    SDL_UpdateTexture(texture, &rect, data, (int)pitch);

    frame_width = width;
    frame_height = height;
}

void frontend_render_present(void)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    if (texture && frame_width && frame_height)
    {
       int out_w, out_h;
       SDL_GetRenderOutputSize(renderer, &out_w, &out_h);

       float ratio = aspect_ratio;
       if (ratio <= 0.0f) {
           ratio = (float)frame_width / (float)frame_height;
       }

       float dst_w = (float)out_w;
       float dst_h = dst_w / ratio;

       if (dst_h > out_h) {
           dst_h = (float)out_h;
           dst_w = dst_h * ratio;
       }

       SDL_FRect src = {0.0f, 0.0f, (float)frame_width, (float)frame_height};
       SDL_FRect dst = 
       {
           (float)(out_w - (int)dst_w) / 2.0f,
           (float)(out_h - (int)dst_h) / 2.0f,
           dst_w,
           dst_h
       };

       SDL_RenderTexture(renderer, texture, &src, &dst);
    }

    SDL_RenderPresent(renderer);

}


void frontend_render_deinit(void)
{
    if (texture)  SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    texture  = NULL;
    renderer = NULL;
    window   = NULL;
    SDL_Quit();
}