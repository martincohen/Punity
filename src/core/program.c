#include <SDL2/SDL.h>
#include "core/core.h"

static Program *_program = 0;

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
        _rec_gif.palette[i * 3 + 0] = _program->palette[i].r;
        _rec_gif.palette[i * 3 + 1] = _program->palette[i].g;
        _rec_gif.palette[i * 3 + 2] = _program->palette[i].b;
    }
}

s32
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
    u32 i, x, y, o;
#if RECORDER_SCALE > 1
    u32 sx, sy;
    Color c;
    u8 *it = _program->_screen;
    for (sy = 0; sy != SCREEN_HEIGHT; ++sy) {
        for (sx = 0; sx != SCREEN_WIDTH; ++sx) {
            c = _program->palette[*it++];
            o = (sy*SCREEN_WIDTH*RECORDER_SCALE*RECORDER_SCALE) + (sx*RECORDER_SCALE);
            for (y = 0; y != RECORDER_SCALE; ++y) {
                for (x = 0; x != RECORDER_SCALE; ++x) {
                    _rec_buffer[o + x + (y*SCREEN_WIDTH*RECORDER_SCALE)] = c;
                }
            }
        }
    }
#else
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        _rec_buffer[i] = _program->palette[_program->_screen[i]];
    }
#endif
    /* Use 30ms delay with 40ms delay every 3 frames, this should match our
     * frame rate of 30fps (~33.3 ms) */
    s32 delay = (_program->frame % 3 == 0) ? 4 : 3;
    jo_gif_frame(&_rec_gif, (void*) _rec_buffer, delay, 0);
}

void
rec_draw(u32 *screen, u32 screen_pitch)
{
    u32 color = _program->palette[(_program->frame & 8) == 0 ? RECORDER_FRAME_COLOR_1 : RECORDER_FRAME_COLOR_2].rgba;
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

#define perf_delta_ms(Last) ((1e3 * (r64)(SDL_GetPerformanceCounter() - Last)) / (r64)perf_freq)
#define perf_delta_us(Last) ((1e6 * (r64)(SDL_GetPerformanceCounter() - Last)) / (r64)perf_freq)

#define perf_collect(P, Out, Samples) \
    P.n++; \
    P.sum += perf_delta_us(P.last); \
    if (P.n == Samples) { \
        Out = (P.sum / (r64)P.n); \
        P.n = 0; \
        P.sum = 0.; \
    }

int main(int argc, char* args[])
{
//    if (argc > 1 && strcmp(args[1], "assets") == 0) {
//        assets_make();
//        return 0;
//    }

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

    _program = malloc(sizeof(Program));
    memset(_program, 0, sizeof(Program));

    _program->screen.data = _program->_screen;
    _program->screen.w = SCREEN_WIDTH;
    _program->screen.h = SCREEN_HEIGHT;
    _program->screen_rect.max_x = _program->screen.w;
    _program->screen_rect.max_y = _program->screen.h;

    _program->bitmap = &_program->screen;
    _program->bitmap_rect = _program->screen_rect;

    _state = &_program->state;

    //
    // Program data.
    //

//    FILE *f = fopen("assets.bin", "rb");
//    assert(f);
//    if (f) {
//        fread(_program->_assets, 1, sizeof(_program->_assets), f);
//        fclose(f);
//    }

    //
    // State initialization.
    //

    _program->buttons[Button_Left].keys[0]  = SDLK_LEFT;
    _program->buttons[Button_Left].keys[1]  = 'a';

    _program->buttons[Button_Right].keys[0] = SDLK_RIGHT;
    _program->buttons[Button_Right].keys[1] = 'd';

    _program->buttons[Button_Up].keys[0] = SDLK_UP;
    _program->buttons[Button_Up].keys[1] = 'w';

    _program->buttons[Button_Down].keys[0] = SDLK_DOWN;
    _program->buttons[Button_Down].keys[1] = 's';

    _program->buttons[Button_A].keys[0] = 'u';
    _program->buttons[Button_A].keys[1] = 'z';

    _program->buttons[Button_B].keys[0] = 'i';
    _program->buttons[Button_B].keys[1] = 'x';

    _program->buttons[Button_X].keys[0] = 'o';
    _program->buttons[Button_X].keys[1] = 'c';

    _program->buttons[Button_Y].keys[0] = 'p';
    _program->buttons[Button_Y].keys[1] = 'v';

    init();
    assert(_program->font);

    //
    // Main loop.
    //

    u8 *it;

    u32 *screen_pixels;
    u32 *screen_it;
    int screen_pitch;
    u32 i;

    u64 perf_freq = SDL_GetPerformanceFrequency();
    u64 perf_time;
    typedef struct PerfTime {
        u64 last;
        u64 sum;
        s32 n;
    } PerfTime;
    PerfTime perf_step = {0};
    PerfTime perf_frame = {0};

//    u64 perf_step_time, perf_step_time_sum, perf_frame_time, perf_now;
//    perf_step_time_sum = 0.;

//    s32 perf_step_time_n = 0;
//    s32 perf_step_time_n = 0;

    r64 time_last = 0;
    r64 time_step = 1. / FPS;

    _program->running = 1;

    Button *button;
    while (_program->running)
    {
        perf_frame.last = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {

            case SDL_QUIT:
                _program->running = 0;
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

                    button = _program->buttons;
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
        perf_collect(perf_step, _program->time_step, 3);

        // Reset button's `Ended` states.
        for (i = 0; i != _Button_MAX; i++) {
            _program->buttons[i].state &= ~(ButtonState_EndedUp | ButtonState_EndedDown);
        }

        // Draw

        assert(SDL_LockTexture(screen_texture, NULL, (void**)&screen_pixels, &screen_pitch) == 0);
        screen_pitch = screen_pitch >> 2;
        // TODO: Do the blit using screen_pitch.
        assert(screen_pitch == SCREEN_WIDTH);
        screen_it = screen_pixels;
        it = _program->_screen;
        for (i = (SCREEN_WIDTH * SCREEN_HEIGHT) + 1;
             i != 0;
             i--) {
            *(screen_it++) = _program->palette[*it++].rgba;
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

        perf_collect(perf_frame, _program->time_frame, 3);

        // Time sync

        r64 time_now = SDL_GetTicks() / 1000.;
        s32 time_wait = (s32)((time_step - (time_now - time_last)) * 1000);
        if (time_wait > 0) {
            time_last += time_step;
            SDL_Delay(time_wait);
        } else {
            time_last = time_now;
        }

        ++_program->frame;

        // printf("wait = %0.3dms, step = %.3fus\n", time_wait, _program->time_step);
    }

#ifdef RECORDER
    rec_end();
#endif

    free(_program);
    SDL_DestroyTexture(screen_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

//#include "core/core.c"
//#include "main.c"
