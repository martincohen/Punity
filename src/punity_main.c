#include <SDL2/SDL.h>
#include "punity_core.h"
#include "punity_assets.h"

Program *PROGRAM = 0;

#ifdef RECORDER
#include <lib/jo_gif.c>

jo_gif_t _rec_gif;
int _rec_active;
Color _rec_buffer[SCREEN_WIDTH * SCREEN_HEIGHT * (RECORDER_SCALE * RECORDER_SCALE)];

void
rec_begin(char *path)
{
    rec_end();
    _rec_active = 1;
    _rec_gif = jo_gif_start(path, SCREEN_WIDTH * RECORDER_SCALE, SCREEN_HEIGHT * RECORDER_SCALE, 0, 15);

    /* jo_gif has been modified to omit the parts that infer the palette.
     * We manually set the palette of the gif here based on our actual palette
     */
    memset(_rec_gif.palette, 0, sizeof(_rec_gif.palette));
    u16 i;
    for (i = 0; i < 256; i++)
    {
        _rec_gif.palette[i * 3 + 0] = PROGRAM->palette[i].r;
        _rec_gif.palette[i * 3 + 1] = PROGRAM->palette[i].g;
        _rec_gif.palette[i * 3 + 2] = PROGRAM->palette[i].b;
    }
}

i32
rec_active()
{
    return _rec_active;
}

void
rec_end(void)
{
    if (!_rec_active) {
        return;
    }

    _rec_active = 0;
    jo_gif_end(&_rec_gif);
}

void
rec_frame()
{
    u32 x, y, o;
#if RECORDER_SCALE > 1
    u32 sx, sy;
    Color c;
    u8 *it = PROGRAM->_screen;
    for (sy = 0; sy != SCREEN_HEIGHT; ++sy) {
        for (sx = 0; sx != SCREEN_WIDTH; ++sx) {
            c = PROGRAM->palette[*it++];
            o = (sy*SCREEN_WIDTH*RECORDER_SCALE*RECORDER_SCALE) + (sx*RECORDER_SCALE);
            for (y = 0; y != RECORDER_SCALE; ++y) {
                for (x = 0; x != RECORDER_SCALE; ++x) {
                    _rec_buffer[o + x + (y*SCREEN_WIDTH*RECORDER_SCALE)] = c;
                }
            }
        }
    }
#else
    u32 i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        _rec_buffer[i] = _program->palette[_program->_screen[i]];
    }
#endif
    /* Use 30ms delay with 40ms delay every 3 frames, this should match our
     * frame rate of 30fps (~33.3 ms) */
    i32 delay = (PROGRAM->frame % 3 == 0) ? 4 : 3;
    jo_gif_frame(&_rec_gif, (void*) _rec_buffer, delay, 0);
}

void
rec_draw(u32 *screen, u32 screen_pitch)
{
    u32 color = PROGRAM->palette[(PROGRAM->frame & 8) == 0 ? RECORDER_FRAME_COLOR_1 : RECORDER_FRAME_COLOR_2].rgba;
    u32 i;
    u32 bottom = screen_pitch * (SCREEN_HEIGHT-1);
    for (i = 0; i != SCREEN_WIDTH; ++i) {
        screen[i] = color;
        screen[i + bottom] = color;
    }
    bottom += screen_pitch;
    for (i = 0; i != bottom; i += screen_pitch) {
        screen[i] = color;
        screen[i + SCREEN_WIDTH - 1] = color;
    }
}

#endif

#define perf_delta_ms(Last) ((1e3 * (f64)(SDL_GetPerformanceCounter() - Last)) / (f64)perf_freq)
#define perf_delta_us(Last) ((1e6 * (f64)(SDL_GetPerformanceCounter() - Last)) / (f64)perf_freq)

#define perf_collect(P, Out, Samples) \
    P.n++; \
    P.sum += perf_delta_us(P.last); \
    if (P.n == Samples) { \
        Out = (P.sum / (f64)P.n); \
        P.n = 0; \
        P.sum = 0.; \
    }

#ifndef ASSETS

int
program_main(int argc, char* argv[])
{
    unused(argc);
    unused(argv);

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

    SDL_Texture *screen_texture = SDL_CreateTexture(renderer,
                                                    // TODO: Use SDL_PIXELFORMAT_RGB565? (16-bit)
                                                    SDL_PIXELFORMAT_ABGR8888,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    SCREEN_WIDTH,
                                                    SCREEN_HEIGHT);

    //
    // Program memory.
    //

    // TODO: VirtualAlloc.
    // TODO: Program stack.

    PROGRAM = calloc(sizeof(Program) + sizeof(ProgramState), 1);

    PROGRAM->screen.data = PROGRAM->_screen;
    PROGRAM->screen.w = SCREEN_WIDTH;
    PROGRAM->screen.h = SCREEN_HEIGHT;
    PROGRAM->screen_rect.max_x = PROGRAM->screen.w;
    PROGRAM->screen_rect.max_y = PROGRAM->screen.h;

    PROGRAM->bitmap = &PROGRAM->screen;
    PROGRAM->bitmap_rect = PROGRAM->screen_rect;

    PROGRAM->state = (ProgramState*)(PROGRAM + 1);

    //
    // Program data.
    //

//    FILE *f = fopen("assets.bin", "rb");
//    ASSERT(f);
//    if (f) {
//        fread(_program->_assets, 1, sizeof(_program->_assets), f);
//        fclose(f);
//    }

    //
    // State initialization.
    //

    PROGRAM->buttons[Button_Left].keys[0]  = SDLK_LEFT;
    PROGRAM->buttons[Button_Left].keys[1]  = 'a';

    PROGRAM->buttons[Button_Right].keys[0] = SDLK_RIGHT;
    PROGRAM->buttons[Button_Right].keys[1] = 'd';

    PROGRAM->buttons[Button_Up].keys[0] = SDLK_UP;
    PROGRAM->buttons[Button_Up].keys[1] = 'w';

    PROGRAM->buttons[Button_Down].keys[0] = SDLK_DOWN;
    PROGRAM->buttons[Button_Down].keys[1] = 's';

    PROGRAM->buttons[Button_A].keys[0] = 'u';
    PROGRAM->buttons[Button_A].keys[1] = 'z';

    PROGRAM->buttons[Button_B].keys[0] = 'i';
    PROGRAM->buttons[Button_B].keys[1] = 'x';

    PROGRAM->buttons[Button_X].keys[0] = 'o';
    PROGRAM->buttons[Button_X].keys[1] = 'c';

    PROGRAM->buttons[Button_Y].keys[0] = 'p';
    PROGRAM->buttons[Button_Y].keys[1] = 'v';

    init(PROGRAM);
    ASSERT(PROGRAM->font);

    //
    // Main loop.
    //

    u8 *it;

    u32 *screen_pixels;
    u32 *screen_it;
    int screen_pitch;
    u32 i;

    u64 perf_freq = SDL_GetPerformanceFrequency();
    typedef struct PerfTime {
        u64 last;
        u64 sum;
        i32 n;
    } PerfTime;
    PerfTime perf_step = {0};
    PerfTime perf_frame = {0};

//    u64 perf_step_time, perf_step_time_sum, perf_frame_time, perf_now;
//    perf_step_time_sum = 0.;

//    s32 perf_step_time_n = 0;
//    s32 perf_step_time_n = 0;

    f64 time_last = 0;
    f64 time_step = 1. / FPS;

    PROGRAM->running = 1;

    Button *button;
    while (PROGRAM->running)
    {
        perf_frame.last = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {

            case SDL_QUIT:
                PROGRAM->running = 0;
                break;

            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                if (!e.key.repeat)
                {
                    if (e.key.state == SDL_PRESSED)
                    {
                        switch (e.key.keysym.sym)
                        {
#ifdef RECORDER
                        case SDLK_F9:
                            if (_rec_active) {
                                rec_end();
                            } else {
                                rec_begin("record.gif");
                            }
                        }
#endif
                    }
                    u8 button_state = e.key.state == SDL_PRESSED
                             ? ButtonState_Down | ButtonState_EndedDown
                             : ButtonState_Up | ButtonState_EndedUp;

                    button = PROGRAM->buttons;
                    for (i = 0; i != _Button_MAX; i++)
                    {
                        if (e.key.keysym.sym == button->keys[0] ||
                            e.key.keysym.sym == button->keys[1] ||
                            e.key.keysym.sym == button->keys[2] ||
                            e.key.keysym.sym == button->keys[3])
                        {
                            button->state = button_state;
                            break;
                        }
                        button++;
                    }
                }
            } break;

            }

            if (e.type == SDL_QUIT) {
                break;
            }
        }

        perf_step.last = SDL_GetPerformanceCounter();
        // ----------------------------------------------------------
        step();
        // ----------------------------------------------------------
        perf_collect(perf_step, PROGRAM->time_step, 3);

        // Reset button's `Ended` states.
        for (i = 0; i != _Button_MAX; i++) {
            PROGRAM->buttons[i].state &= ~(ButtonState_EndedUp | ButtonState_EndedDown);
        }

        // Draw

        ASSERT(SDL_LockTexture(screen_texture, NULL, (void**)&screen_pixels, &screen_pitch) == 0);
        screen_pitch = screen_pitch >> 2;
        // TODO: Do the blit using screen_pitch.
        ASSERT(screen_pitch == SCREEN_WIDTH);
        screen_it = screen_pixels;
        it = PROGRAM->_screen;
        for (i = (SCREEN_WIDTH * SCREEN_HEIGHT) + 1;
             i != 0;
             i--) {
            *(screen_it++) = PROGRAM->palette[*it++].rgba;
        }
#ifdef RECORDER
        if (_rec_active) {
            rec_frame();
            rec_draw(screen_pixels, screen_pitch);
        }
#endif
        SDL_UnlockTexture(screen_texture);

        // SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        perf_collect(perf_frame, PROGRAM->time_frame, 3);

        // Time sync

        f64 time_now = SDL_GetTicks() / 1000.;
        i32 time_wait = (i32)((time_step - (time_now - time_last)) * 1000);
        if (time_wait > 0) {
            time_last += time_step;
            SDL_Delay(time_wait);
        } else {
            time_last = time_now;
        }

        ++PROGRAM->frame;

        // printf("wait = %0.3dms, step = %.3fus\n", time_wait, _program->time_step);
    }

#ifdef RECORDER
    rec_end();
#endif

    free(PROGRAM);
    SDL_DestroyTexture(screen_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#endif

int
main(int argc, char* argv[])
{
#ifdef ASSETS
    return assets_main(argc, argv);
#else
    if (argc > 1 && strcmp(argv[1], "assets") == 0) {
        printf("Running assets compiler...");
        return assets_main(argc, argv);
    } else {
        return program_main(argc, argv);
    }
#endif
}

//#include "punity_core.c"
//#include "main.c"
