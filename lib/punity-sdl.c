#include "SDL.h"
#include "SDL_opengl.h"

static struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GLContext *context;

    GLuint texture;
}
punp_runtime_sdl;

bool
window_is_fullscreen()
{
    Uint32 flags = SDL_GetWindowFlags(punp_runtime_sdl.window);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void
window_fullscreen_toggle()
{
    if (window_is_fullscreen()) {
        SDL_SetWindowFullscreen(punp_runtime_sdl.window, 0);
    } else {
        SDL_SetWindowFullscreen(punp_runtime_sdl.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void
window_resize(int width, int height, int scale)
{
    CORE->window.width = width;
    CORE->window.height = height;
    CORE->window.scale_normal = scale;
    SDL_SetWindowSize(punp_runtime_sdl.window,
        width * scale, height * scale);
}

void
window_title(const char *title)
{
    SDL_SetWindowTitle(punp_runtime_sdl.window, title);
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
#define PUN_KEY_MAPPING_ENTRY(k, v, s) { KEY_##k, SDLK_##k },
PUN_KEY_MAPPING
#undef PUN_KEY_MAPPING_ENTRY
    };

    u8 *vkey = pun_sdl_remap_vkey_table;
    u8 *scan = pun_sdl_remap_scan_table;
	SDL_Log("%d", array_count(table));
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

extern int
main(int argc, char **argv)
{
	pun_sdl_make_key_maps();

    // TODO: Concat `argv`.
    const char *args = "";
    punity_init(args);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    i32 window_width  = CORE->window.width * CORE->window.scale_normal;
    i32 window_height = CORE->window.height * CORE->window.scale_normal;
    punp_runtime_sdl.window = SDL_CreateWindow(
        "Punity (SDL)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width, window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (punp_runtime_sdl.window == 0) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

#if PUN_CONSOLE
    // printf("Debug build...");
    // if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

#if 0
    punp_runtime_sdl.renderer = SDL_CreateRenderer(
        punp_runtime_sdl.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (punp_runtime_sdl.renderer == 0) {
        SDL_DestroyWindow(punp_runtime_sdl.window);
        SDL_Log("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
#endif

    punp_runtime_sdl.context = SDL_GL_CreateContext(punp_runtime_sdl.window);

    if (SDL_GL_SetSwapInterval(1) < 0) {
        printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
    }

    int display_refresh_rate = 0;
    SDL_DisplayMode display_mode;
    int display_index = SDL_GetWindowDisplayIndex(punp_runtime_sdl.window);
    if (SDL_GetDesktopDisplayMode(display_index, &display_mode) == 0) {
        if (display_mode.refresh_rate != 0) {
            display_refresh_rate = display_mode.refresh_rate;
        }
    }

    if (display_refresh_rate == 0) {
        printf("Warning: Display refresh rate is unknown.\n");
        display_refresh_rate = 60;
    } else {
        printf("Display refresh rate: %d\n", display_refresh_rate);
    }

    glViewport(0.f, 0.f, window_width, window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, 1.0, -1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glGenTextrues(1, &punp_runtime_sdl.texture);
    glBindTexture(GL_TEXTURE_2D, punp_runtime_sdl.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // init();

    SDL_StartTextInput();

    GLuint *window_buffer = bank_push(CORE->storage, CORE->window.width * CORE->window.height * sizeof(u32));
    GLuint *window_row;
    u8  *canvas_it;

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
                if (key = pun_sdl_remap_sym(sdl_event.key.keysym.sym)) {
                    punity_on_key_down(key, 0);
                }
                break;
            case SDL_KEYUP:
                if (key = pun_sdl_remap_sym(sdl_event.key.keysym.sym)) {
                    punity_on_key_up(key, 0);
                }
                break;
            case SDL_TEXTINPUT:
                punity_on_text(sdl_event.text.text);
                break;
            case SDL_WINDOWEVENT:
                if (sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window_width  = sdl_event.window.data1;
                    window_height = sdl_event.window.data2;
                    printf("width = %d, height = %d\n", window_width, window_height);
                    glViewport(0.f, 0.f, window_width, window_height);
                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    glOrtho(0.0, window_width, window_height, 0.0, 1.0, -1.0);
                }
                break;
            }
        }

        SDL_GetMouseState(&x, &y);
        CORE->mouse_x = ((f32)x) / CORE->window.scale_current;
        CORE->mouse_y = ((f32)y) / CORE->window.scale_current;

        punity_frame_step();

        glClear(GL_COLOR_BUFFER_BIT);

        window_row = window_buffer;
        canvas_it = CORE->canvas.bitmap->pixels;
        for (y = 0; y != CORE->window.height; ++y) {
            for (x = 0; x != CORE->window.width; ++x) {
                *(window_row++) = CORE->palette->colors[*canvas_it++].rgba;
            }
        }

        glBindTexture(GL_TEXTURE_2D, punp_runtime_sdl.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CORE->window.width, CORE->window.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, window_buffer);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(window_width, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(window_width, window_height);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, window_height);
        glEnd();

        SDL_GL_SwapWindow(punp_runtime_sdl.window);

        punity_frame_end();
    }
    terminate();

    return 0;
}
