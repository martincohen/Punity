#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#define internal static

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

typedef struct {
    union {
        struct
        {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };
        u32 rgba;
    };
} Pixel32;

//
//
//

SDL_Surface *gsurface;

#if 0
SDL_Color gpalette[256];

void
palette_set(Pixel32 *palette, u8 count)
{
    for (u8 i = 0; i != count; ++i)
    {
        gpalette[i].r = palette[i].r;
        gpalette[i].g = palette[i].g;
        gpalette[i].b = palette[i].b;
    }
    SDL_SetPaletteColors(gsurface->format->palette, color, 0, 256);
}
#endif

int main(int argc, char* args[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("MLP",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    
    if (window == 0) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);


    gsurface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0, 0, 0, 0);

    SDL_Color gpalette[256] = {
        { 255, 0, 0, 255 },
        { 255, 255, 0, 255 },
        { 255, 255, 255, 255 }
    };

    u8 *surface_pixels = (u8*)gsurface->lock_data;
    surface_pixels[0] = 0;
    surface_pixels[1] = 1;
    surface_pixels[2] = 2;

    // SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, gsurface);

    SDL_Texture *screen_texture = SDL_CreateTexture(renderer,
                                                    // TODO: Use SDL_PIXELFORMAT_RGB565? (16-bit)
                                                    SDL_PIXELFORMAT_RGBA8888,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    SCREEN_WIDTH,
                                                    SCREEN_HEIGHT);
    SDL_Color *screen_pixels;
    int screen_pitch;
    u32 i;

    for (;;)
    {
        SDL_Event e;
        if (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                break;
            }
        }

        SDL_LockTexture(screen_texture, NULL, (void**)&screen_pixels, &screen_pitch);
        for (i = (SCREEN_WIDTH * SCREEN_HEIGHT) + 1;
             i != 0;
             i++) {
            *(screen_pixels++) = gpalette[*surface_pixels++];
        }
        SDL_UnlockTexture(screen_texture);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(screen_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
