#ifndef PUNITY_RUNTIME_SDL_INCLUDED
#define PUNITY_RUNTIME_SDL_INCLUDED

#include "SDL.h"
#include "SDL_opengl.h"

typedef struct
{
    char window_init_title[256];
    int window_init_flags;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GLContext *context;

    GLuint texture;
    SDL_mutex *audio_mutex;
    SDL_AudioSpec audio_spec;
    SDL_AudioDeviceID audio_device;
    Bank audio_bank;
    Uint8* audio_buffer;
    f64 audio_buffer_time;
    f64 audio_time;
    f64 game_time;
}
PunitySDLRuntime_;

#endif // PUNITY_RUNTIME_SDL_INCLUDED

#ifdef PUNITY_RUNTIME_SDL_IMPLEMENTATION
#undef PUNITY_RUNTIME_SDL_IMPLEMENTATION

static PunitySDLRuntime_ sdl_;

// #if PUNITY_PLATFORM_LINUX || PUNITY_PLATFORM_OSX
// #endif

bool
window_is_fullscreen()
{
    Uint32 flags = SDL_GetWindowFlags(sdl_.window);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void
window_fullscreen_toggle()
{
    if (sdl_.window) {
        if (window_is_fullscreen()) {
            SDL_SetWindowFullscreen(sdl_.window, 0);
        } else {
            SDL_SetWindowFullscreen(sdl_.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
    } else {
        sdl_.window_init_flags = !sdl_.window_init_flags;
    }
}

void
window_resize(int width, int height, int scale)
{
    CORE->window.width = width;
    CORE->window.height = height;
    CORE->window.scale = scale;
    SDL_SetWindowSize(sdl_.window,
        width * scale, height * scale);
}

void
window_title(const char *title)
{
    if (sdl_.window) {
        SDL_SetWindowTitle(sdl_.window, title);
    } else {
        int length = minimum(array_count(sdl_.window_init_title) - 1, strlen(title));
        memcpy(sdl_.window_init_title, title, length);
        sdl_.window_init_title[length] = 0;
    }

}

static u8 pun_sdl_remap_vkey_table[256] = {0};
static u8 pun_sdl_remap_scan_table[SDL_NUM_SCANCODES] = {0};

enum {
    SDLK_A = SDLK_a, SDLK_B = SDLK_b, SDLK_C = SDLK_c, SDLK_D = SDLK_d,
    SDLK_E = SDLK_e, SDLK_F = SDLK_f, SDLK_G = SDLK_g, SDLK_H = SDLK_h,
    SDLK_I = SDLK_i, SDLK_J = SDLK_j, SDLK_K = SDLK_k, SDLK_L = SDLK_l,
    SDLK_M = SDLK_m, SDLK_N = SDLK_n, SDLK_O = SDLK_o, SDLK_P = SDLK_p,
    SDLK_Q = SDLK_q, SDLK_R = SDLK_r, SDLK_S = SDLK_s, SDLK_T = SDLK_t,
    SDLK_U = SDLK_u, SDLK_V = SDLK_v, SDLK_W = SDLK_w, SDLK_X = SDLK_x,
    SDLK_Y = SDLK_y, SDLK_Z = SDLK_z,
    SDLK_APOSTROPHE = SDLK_QUOTE,
    SDLK_EQUAL = SDLK_EQUALS,
    SDLK_LEFT_BRACKET = SDLK_LEFTBRACKET,
    SDLK_RIGHT_BRACKET = SDLK_RIGHTBRACKET,
    SDLK_BACK_QUOTE = SDLK_BACKQUOTE,
    SDLK_BACK_SLASH = SDLK_BACKSLASH,
    SDLK_MOUSE_LEFT = 0,
    SDLK_MOUSE_RIGHT = 0,
    SDLK_MOUSE_MIDDLE = 0,
};

void
pun_sdl_make_key_maps()
{

    struct { int key, sym; } table[] = {
#define PUNITY_KEY_MAPPING_ENTRY(k, v, s) { KEY_##k, SDLK_##k },
PUNITY_KEY_MAPPING
#undef PUNITY_KEY_MAPPING_ENTRY
    };

    u8 *vkey = pun_sdl_remap_vkey_table;
    u8 *scan = pun_sdl_remap_scan_table;
	LOG("%d", (uint)array_count(table));
    for (int i = 0; i != array_count(table); ++i) {
        if (table[i].sym & (1 << 30)) {
            int s = table[i].sym & ~(1 << 30);
            ASSERT(s < array_count(pun_sdl_remap_scan_table));
            scan[s] = table[i].key;
        } else {
            ASSERT(table[i].sym < array_count(pun_sdl_remap_vkey_table));
            vkey[table[i].sym] = table[i].key;
        }
    }
}

u8
pun_sdl_remap_sym(int sym)
{
    if (sym & (1 << 30)) {
        sym = sym & ~(1 << 30);
        if (sym > array_count(pun_sdl_remap_scan_table)) {
            return KEY_UNKNOWN;
        }
        return pun_sdl_remap_scan_table[sym];
    } else {
        if (sym > array_count(pun_sdl_remap_vkey_table)) {
            return KEY_UNKNOWN;
        }
        return pun_sdl_remap_vkey_table[sym];
    }
}

//
// Audio
//

void
punsdl_audio_mutex_lock_() {
    SDL_LockMutex(sdl_.audio_mutex);
}

void
punsdl_audio_mutex_unlock_() {
    SDL_UnlockMutex(sdl_.audio_mutex);
}

// void
// punsdl_audio_callback_(void *userdata, Uint8* stream, int len)
// {
//     LOG("Mixing...\n");
//     punsdl_audio_mutex_lock_();
//     SDL_memset(stream, 0, len);
//     sound_mix_(&sdl_.audio_bank, (i16*)stream, PUNP_SOUND_BYTES_TO_SAMPLES(len, PUNP_SOUND_CHANNELS));
//     punsdl_audio_mutex_unlock_();
// }

extern int
main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER) != 0) {
        FAIL("SDL_Init (%s)", SDL_GetError());
        return 0;
    }

#if 0
    mtar_t tar;
    mtar_header_t h;

    // TODO: Get binary path.
    mtar_open(&tar, "res.tar", "r");
    while ( (mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD ) {
        LOG("%s (%d bytes)\n", h.name, h.size);
        mtar_next(&tar);
    }
    mtar_close(&tar);
#endif

	pun_sdl_make_key_maps();

    // TODO: Concat `argv`.
    const char *args = "";
    punity_init(args);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    i32 window_width  = CORE->window.width * CORE->window.scale;
    i32 window_height = CORE->window.height * CORE->window.scale;
    sdl_.window = SDL_CreateWindow(
        "Punity (SDL)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width, window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (sdl_.window == 0) {
        LOG("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (*sdl_.window_init_title) {
        SDL_SetWindowTitle(sdl_.window, sdl_.window_init_title);
    }

    if (sdl_.window_init_flags) {
        SDL_SetWindowFullscreen(sdl_.window, SDL_WINDOW_FULLSCREEN_DESKTOP);        
    }

#if PUNITY_CONSOLE
    // LOG("Debug build...");
    // if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    sdl_.audio_mutex = SDL_CreateMutex();
    // For the audio mixing thread.
    bank_init(&sdl_.audio_bank, megabytes(2));

    SDL_AudioSpec audio_spec_req = {0};
    audio_spec_req.freq = 48000; // 44100;
    audio_spec_req.format = AUDIO_S16;
    audio_spec_req.channels = 2;
    audio_spec_req.samples = 4096;
    audio_spec_req.callback = 0; // punsdl_audio_callback_;

    sdl_.audio_device = SDL_OpenAudioDevice(NULL,
        0, &audio_spec_req,
        &sdl_.audio_spec,
        0); // Allow no changes.
    
    if (sdl_.audio_device != 0)
    {
        LOG("audio.size = %d\n"
               "audio.padding = %d\n"
               "audio.silence = %d\n"
               "audio.channels = %d\n"
               "audio.format = %d\n"
               "audio.freq = %d\n"
               "audio_update = %.3lf\n",
               (int)sdl_.audio_spec.size,
               (int)sdl_.audio_spec.padding,
               (int)sdl_.audio_spec.silence,
               (int)sdl_.audio_spec.channels,
               (int)sdl_.audio_spec.format,
               (int)sdl_.audio_spec.freq,
               sdl_.audio_buffer_time);

        sdl_.audio_buffer_time = (f64)audio_spec_req.samples / (f64)audio_spec_req.freq;
        // sdl_.audio_update_time_delta = 0;
        sdl_.audio_buffer = bank_push(&sdl_.audio_bank, sdl_.audio_spec.samples * sizeof(i16) * sdl_.audio_spec.channels);
        ASSERT(sdl_.audio_spec.freq == 48000);
        ASSERT(sdl_.audio_spec.format == AUDIO_S16);
        SDL_PauseAudioDevice(sdl_.audio_device, 0);
    }
    else
    {
#if GAME_DEBUG
        FAIL("Failed to initialize audio engine. (%s)", SDL_GetError());
#endif
    }


#if 0
    sdl_.renderer = SDL_CreateRenderer(
        sdl_.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (sdl_.renderer == 0) {
        SDL_DestroyWindow(sdl_.window);
        SDL_Log("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
#endif

    sdl_.context = SDL_GL_CreateContext(sdl_.window);

    if (SDL_GL_SetSwapInterval(1) < 0) {
        LOG("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
    }

    int display_refresh_rate = 0;
    SDL_DisplayMode display_mode;
    int display_index = SDL_GetWindowDisplayIndex(sdl_.window);
    if (SDL_GetDesktopDisplayMode(display_index, &display_mode) == 0) {
        if (display_mode.refresh_rate != 0) {
            display_refresh_rate = display_mode.refresh_rate;
        }
    }

    if (display_refresh_rate == 0) {
        LOG("Warning: Display refresh rate is unknown.\n");
        display_refresh_rate = 60;
    } else {
        LOG("Display refresh rate: %d\n", display_refresh_rate);
    }

    f32 window_scaled_width  = CORE->window.width  * CORE->window.scale;
    f32 window_scaled_height = CORE->window.height * CORE->window.scale;

    glViewport(CORE->window.viewport_min_x, CORE->window.viewport_min_y,
        window_scaled_width, window_scaled_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(0.0, window_scaled_width, window_scaled_height, 0.0, 1.0, -1.0);
    glOrtho(0.0, window_scaled_width, 0.0, window_scaled_height, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &sdl_.texture);
    glBindTexture(GL_TEXTURE_2D, sdl_.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // init();

    int width, height;
    SDL_GetWindowSize(sdl_.window, &width, &height);
    punity_viewport_update(width, height);

    SDL_StartTextInput();

    GLuint *window_buffer = bank_push(CORE->storage, CORE->window.width * CORE->window.height * sizeof(u32));
    GLuint *window_row;
    u8  *canvas_it;

    sdl_.audio_time = 0;
    sdl_.game_time = 0;

    int x, y;
    u8 key;
    SDL_Event sdl_event;
    while (CORE->running)
    {
        punity_frame_begin();

        while (SDL_PollEvent(&sdl_event) != 0)
        {
            switch(sdl_event.type)
            {
            case SDL_QUIT:
                CORE->running = 0;
                break;
            case SDL_KEYDOWN:
                if ((key = pun_sdl_remap_sym(sdl_event.key.keysym.sym))) {
                    punity_on_key_down(key, 0);
                }
                break;
            case SDL_KEYUP:
                if ((key = pun_sdl_remap_sym(sdl_event.key.keysym.sym))) {
                    punity_on_key_up(key, 0);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (sdl_event.button.button)
                {
                case SDL_BUTTON_LEFT:
                    punity_on_key_down(KEY_MOUSE_LEFT, 0);
                    break;
                case SDL_BUTTON_RIGHT:
                    punity_on_key_down(KEY_MOUSE_RIGHT, 0);
                    break;
                case SDL_BUTTON_MIDDLE:
                    punity_on_key_down(KEY_MOUSE_MIDDLE, 0);
                    break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch (sdl_event.button.button)
                {
                case SDL_BUTTON_LEFT:
                    punity_on_key_up(KEY_MOUSE_LEFT, 0);
                    break;
                case SDL_BUTTON_RIGHT:
                    punity_on_key_up(KEY_MOUSE_RIGHT, 0);
                    break;
                case SDL_BUTTON_MIDDLE:
                    punity_on_key_up(KEY_MOUSE_MIDDLE, 0);
                    break;
                }
                break;
            case SDL_TEXTINPUT:
                punity_on_text(sdl_event.text.text);
                break;
            case SDL_WINDOWEVENT:
                if (sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    // window_width  = sdl_event.window.data1;
                    // window_height = sdl_event.window.data2;
                    punity_viewport_update(sdl_event.window.data1, sdl_event.window.data2);
                }
                break;
            }
        }

        SDL_GetMouseState(&x, &y);
        CORE->mouse_x = ((f32)x - CORE->window.viewport_min_x) / CORE->window.scale;
        CORE->mouse_y = ((f32)y - CORE->window.viewport_min_y) / CORE->window.scale;

        punity_frame_step();

        // @lclhstr had problems with audio.
        // @miblo_ hears a loop-end glitch.

        // Step the sound.
        if (sdl_.audio_device != 0)
        {
            // sdl_.game_time += CORE->time_delta;
            // LOG("game %.3lf audio %.3lf (size = %d)\n", sdl_.game_time, sdl_.audio_time, );
            // f64 audio_prefill_frames = 0.033 * 2.0;
            f64 time_needed = sdl_.audio_spec.samples / (f64)sdl_.audio_spec.freq; // 0.033 * 2.5;
            int queued_samples = SDL_GetQueuedAudioSize(sdl_.audio_device) / (sizeof(i16) * sdl_.audio_spec.channels);
            f64 queued_time = (f64)queued_samples / (f64)sdl_.audio_spec.freq;
            // LOG("queued time %.3lf\n", queued_time);
            if (queued_time < time_needed) {
                int samples_needed = (int)((f64)sdl_.audio_spec.freq * time_needed);
                samples_needed = clamp(samples_needed, 0, sdl_.audio_spec.samples);
                if (samples_needed > 0) {
                    size_t size_needed = samples_needed * sizeof(i16) * sdl_.audio_spec.channels;
                    sound_mix_(&sdl_.audio_bank, (i16*)sdl_.audio_buffer, size_needed, sdl_.audio_spec.channels);
                    SDL_QueueAudio(sdl_.audio_device, sdl_.audio_buffer, size_needed);
                    sdl_.audio_time += (f64)samples_needed / (f64)sdl_.audio_spec.freq;
                    // LOG("audio FILL %.3lf\n", (f64)samples_needed / sdl_.audio_spec.freq);
                }
            }
        }

        f32 window_scaled_width  = CORE->window.width  * CORE->window.scale;
        f32 window_scaled_height = CORE->window.height * CORE->window.scale;

        glViewport(CORE->window.viewport_min_x, CORE->window.viewport_min_y,
            window_scaled_width, window_scaled_height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, window_scaled_width, window_scaled_height, 0.0, 1.0, -1.0);
        // glOrtho(0.0, window_scaled_width, 0.0, window_scaled_height, 1.0, -1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClear(GL_COLOR_BUFFER_BIT);

        window_row = window_buffer;
        canvas_it = CORE->canvas.bitmap->pixels;
        for (y = 0; y != CORE->window.height; ++y) {
            for (x = 0; x != CORE->window.width; ++x) {
                *(window_row++) = CORE->palette->colors[*canvas_it++].rgba;
            }
        }

        glBindTexture(GL_TEXTURE_2D, sdl_.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CORE->window.width, CORE->window.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, window_buffer);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(window_scaled_width, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(window_scaled_width, window_scaled_height);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, window_scaled_height);
        glEnd();

        SDL_GL_SwapWindow(sdl_.window);

        punity_frame_end();
    }

    if (sdl_.audio_device) {
        SDL_CloseAudioDevice(sdl_.audio_device);
    }

    SDL_GL_DeleteContext(sdl_.context);
    SDL_DestroyWindow(sdl_.window);
    SDL_Quit();

    return 0;
}

#endif // PUNITY_RUNTIME_SDL_IMPLEMENTATION