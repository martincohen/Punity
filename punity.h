#ifndef PUNITY_H
#define PUNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "config.h"

#define PLATFORM_WINDOWS 1

#ifndef SOUND_CHANNELS
#define SOUND_CHANNELS 4
#endif

#ifndef SOUND_SAMPLE_RATE
#define SOUND_SAMPLE_RATE 48000
#endif

#ifndef CANVAS_WIDTH
#define CANVAS_WIDTH 128
#endif

#ifndef CANVAS_HEIGHT
#define CANVAS_HEIGHT 128
#endif

#ifndef CANVAS_SCALE
#define CANVAS_SCALE 2
#endif

#ifndef STACK_CAPACITY
#define STACK_CAPACITY megabytes(1)
#endif

#ifndef STORAGE_CAPACITY
#define STORAGE_CAPACITY megabytes(1)
#endif

#ifndef USE_STB_IMAGE
#define USE_STB_IMAGE 0
#endif

#ifndef USE_STB_VORBIS
#define USE_STB_VORBIS 0
#endif

#if PLATFORM_WINDOWS
#define COLOR_CHANNELS b, g, r, a
#endif

typedef uint8_t  u8;
typedef int8_t   i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;
typedef u32      b32;

#define unused(x) (void)x
#define align_to(value, N) ((value + (N-1)) & ~(N-1))

#define maximum(a, b) (a) > (b) ? (a) : (b)
#define minimum(a, b) (a) < (b) ? (a) : (b)
#define clamp(x, a, b)  (MAX(a, MIN(x, b)))

#if 0
#if defined(_MSC_VER)

// #define maximum(a, b) \
//   ({ decltype(a) _a = (a); \
//      decltype(b) _b = (b); \
//     _a > _b ? _a : _b; \
//   })

// #define minimum(a, b) \
//   ({ decltype(a) _a = (a); \
//      decltype(b) _b = (b); \
//     _a < _b ? _a : _b; \
//   })

#elif defined(__GCC__)

#define maximum(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; \
  })

#define minimum(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; \
  })
#endif
#endif

//
// Utility
//

typedef struct 
{
    f64 stamp;
    f32 delta;
}
PerfSpan;

f64 perf_get();

// Sets the stamp, keeps the delta.
//
void perf_from(PerfSpan *span);

// Sets the stamp and delta.
//
void perf_to(PerfSpan *span);

// Returns current delta.
f32 perf_delta(const PerfSpan *span);

//
// Memory
//

#define kilobytes(n) (1024*(n))
#define megabytes(n) (1024*kilobytes(n))
#define gigabytes(n) (1024*megabytes(n))

typedef struct
{
    u8 *begin;
    u8 *end;
    u8 *it;
}
Bank;

void *virtual_reserve(void* ptr, u32 size);
void *virtual_commit(void *ptr, u32 size);
void virtual_decommit(void *ptr, u32 size);
void virtual_free(void *ptr, u32 size);
void *virtual_alloc(void* ptr, u32 size);

void bank_init(Bank *bank, u32 capacity);
void *bank_push(Bank *bank, u32 size);
void bank_pop(Bank *bank, void *ptr);

typedef struct
{
    Bank state;
    Bank *bank;
}
BankState;

BankState bank_begin(Bank *bank);
void bank_end(BankState *state);

//
// File I/O
//

void *file_read(const char *path, size_t *size);

//
//
//

#define COLOR_TRANSPARENT 0

typedef union
{
    struct
    {
        u8 COLOR_CHANNELS;
    };
    u32 rgba;
}
Color;

Color color_make(u8 r, u8 g, u8 b, u8 a);

typedef struct
{
    Color colors[256];
    u8 colors_count;
}
Palette;

typedef union
{
    struct {
        i32 min_x;
        i32 min_y;
        i32 max_x;
        i32 max_y;
    };
    struct {
        i32 left;
        i32 top;
        i32 right;
        i32 bottom;
    };
    i32 E[4];
}
Rect;

#define rect_tr(r, tx, ty) \
    (r)->min_x += tx; \
    (r)->min_y += ty; \
    (r)->max_x += tx; \
    (r)->max_y += ty;

inline Rect rect_make(i32 min_x, i32 min_y, i32 max_x, i32 max_y);
inline Rect rect_make_size(i32 x, i32 y, i32 w, i32 h);

typedef struct
{
    i32 width;
    i32 height;
    u8 *pixels;
}
Bitmap;

typedef struct
{
    Bitmap bitmap;
    i32 char_width;
    i32 char_height;
}
Font;

#define BITMAP_8  1
#define BITMAP_32 4

// Initializes the bitmap struct and copies the data from pixels to the CORE->storage.
// If pixels is 0, the bitmap's pixel data are only allocated (not initialized)
// If pixels is not 0 then the `type` is used as follows:
//
// - BITMAP_32, it'll convert it to paletted image by adding the unknown colors to the palette.
// - BITMAP_8,  the data are copied as they are.
//
void bitmap_init(Bitmap *bitmap, i32 width, i32 height, void *pixels, int type);
void bitmap_clear(Bitmap *bitmap, u8 color);

// Saves bitmap to file. Uses simple format:
//
// 0x0000 i32 width
// 0x0004 i32 height
// 0x0008 u8  pixels[width * height]
//
void bitmap_save_raw(Bitmap *bitmap, FILE *file);

// Loads bitmap from raw data format (see bitmap_save_raw())
//
void bitmap_load_raw(Bitmap *bitmap, FILE *file);

// Loads bitmap from an image file.
// This only works if USE_STB_IMAGE is defined.
#if USE_STB_IMAGE
void bitmap_load(Bitmap *bitmap, const char *path);
void bitmap_load_resource(Bitmap *bitmap, const char *resource_name);
#endif

//
//
//

// Clears the canvas.
void canvas_clear(u8 color);

// Sets clipping rectangle.
// The rectangle is intersected with the canvas rectangle [0, 0, canvas.width, canvas.height].
void clip_set(Rect rect);
// Resets clip to current canvas bitmap.
void clip_reset();
// Checks clipping rectangle whether it's valid.
bool clip_check();

// Produces a new palette B from palette A by moving colors.
// This is useful if you want to lighten or darken the colors.
// `ramp` is u8[256] array of indexes starting at darkest color as 1.
// Index of 0 means the color is not being shifted.
void palette_shift(Palette *A, Palette *B, u8 *ramp, i32 amount);

enum {
    DrawFlags_FlipH = 0x01,
    DrawFlags_Mask  = 0x02
};

// Draws a bitmap to the canvas.
void bitmap_draw(i32 x, i32 y, i32 pivot_x, i32 pivot_y, Bitmap *bitmap, Rect *bitmap_rect, u32 flags, u8 color);

// Draws text to the canvas.
// Fails if CORE->font is not set, as it's using it draw the text.
void text_draw(i32 x, i32 y, const char *text, u8 color);

//
// Sound
//

typedef struct
{
    i32 channel_count;
    u32 rate;
    i16 *samples;
    // Samples per channel.
    size_t samples_count;
}
Sound;

void sound_play(Sound *sound);

#if USE_STB_VORBIS
void sound_load(Sound *sound, const char *path);
void sound_load_resource(Sound *sound, const char *resource_name);
#endif

//
// Resources
//

void *resource_get(const char *name, size_t *size);

//
// Core
//

#define KEYS_MAX 256

typedef struct
{
    // Memory for large in-function working data.
    // Use STACK_CAPACITY macro in config.h to set size.
    // Ex. #define STACK_CAPACITY megabytes(4)
    //
    // Use this to allocate the data in the storage.
    // void *my_ptr = bank_push(stack, 65536);
    //
    // Use this to deallocate the data from the storage.
    // Don't forget to do pop before the function returns.
    // bank_pop(stack, my_ptr);
    //
    Bank *stack;

    // Storage for the application data (for example bitmaps).
    // Use STACK_STORAGE macro in config.h to set size.
    // Ex. #define STACK_CAPACITY megabytes(4)
    //
    Bank *storage;

    // Set to 0 to exit.
    //
    b32 running;

    u32 key_modifiers;
    
    // Indexed with KEY_* constants.
    // 0 for up
    // 1 for down
    //
    u8 key_states[KEYS_MAX];

    // Indexed with KEY_* constants.
    // 0 for not changed in this frame
    // 1 for changed in this frame
    //
    u8 key_deltas[KEYS_MAX];

    // Total frame time.
    PerfSpan perf_frame;
	// Total time taken to process everything except the vsync.
	PerfSpan perf_frame_inner;

	// Total time taken to step.
    PerfSpan perf_step;
	// Total time taken to process audio.
	PerfSpan perf_audio;
    // Total time taken to do the blit.
    PerfSpan perf_blit;

    // Current frame number.
    i64 frame;

    Palette palette;
    Bitmap *canvas;
    i32 translate_x;
    i32 translate_y;
    Rect clip;

    Font *font;
}
Core;

extern Core *CORE;

void panic(const char *message, const char *description, const char *function, const char *file, int line);

#define ASSERT_MESSAGE(expression, message) \
    (void)( (!!(expression)) ||\
            (panic(message, #expression, __FUNCTION__, __FILE__, __LINE__), 0) )

#define ASSERT(expression) ASSERT_MESSAGE(expression, #expression)
#define ASSERT_DEBUG_MESSAGE ASSERT_MESSAGE
#define ASSERT_DEBUG ASSERT

// To be defined in main.c.
void init();
// To be defined in main.c.
void step();

//
// Keys
//

#define KEY_MOD_SHIFT           0x0001
#define KEY_MOD_CONTROL         0x0002
#define KEY_MOD_ALT             0x0004
#define KEY_MOD_SUPER           0x0008

#define KEY_MOD_SHIFT           0x0001
#define KEY_MOD_CONTROL         0x0002
#define KEY_MOD_ALT             0x0004
#define KEY_MOD_SUPER           0x0008

#define KEY_UNKNOWN            -1
#define KEY_INVALID            -2

#define KEY_SPACE              32
#define KEY_APOSTROPHE         39  /* ' */
#define KEY_COMMA              44  /* , */
#define KEY_MINUS              45  /* - */
#define KEY_PERIOD             46  /* . */
#define KEY_SLASH              47  /* / */
#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57
#define KEY_SEMICOLON          59  /* ; */
#define KEY_EQUAL              61  /* = */
#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90
#define KEY_LEFT_BRACKET       91  /* [ */
#define KEY_BACKSLASH          92  /* \ */
#define KEY_RIGHT_BRACKET      93  /* ] */
#define KEY_GRAVE_ACCENT       96  /* ` */
#define KEY_WORLD_1            161 /* non-US #1 */
#define KEY_WORLD_2            162 /* non-US #2 */

#define KEY_ESCAPE             256
#define KEY_ENTER              257
#define KEY_TAB                258
#define KEY_BACKSPACE          259
#define KEY_INSERT             260
#define KEY_DELETE             261
#define KEY_RIGHT              262
#define KEY_LEFT               263
#define KEY_DOWN               264
#define KEY_UP                 265
#define KEY_PAGE_UP            266
#define KEY_PAGE_DOWN          267
#define KEY_HOME               268
#define KEY_END                269
#define KEY_CAPS_LOCK          280
#define KEY_SCROLL_LOCK        281
#define KEY_NUM_LOCK           282
#define KEY_PRINT_SCREEN       283
#define KEY_PAUSE              284
#define KEY_F1                 290
#define KEY_F2                 291
#define KEY_F3                 292
#define KEY_F4                 293
#define KEY_F5                 294
#define KEY_F6                 295
#define KEY_F7                 296
#define KEY_F8                 297
#define KEY_F9                 298
#define KEY_F10                299
#define KEY_F11                300
#define KEY_F12                301
#define KEY_F13                302
#define KEY_F14                303
#define KEY_F15                304
#define KEY_F16                305
#define KEY_F17                306
#define KEY_F18                307
#define KEY_F19                308
#define KEY_F20                309
#define KEY_F21                310
#define KEY_F22                311
#define KEY_F23                312
#define KEY_F24                313
#define KEY_F25                314
#define KEY_KP_0               320
#define KEY_KP_1               321
#define KEY_KP_2               322
#define KEY_KP_3               323
#define KEY_KP_4               324
#define KEY_KP_5               325
#define KEY_KP_6               326
#define KEY_KP_7               327
#define KEY_KP_8               328
#define KEY_KP_9               329
#define KEY_KP_DECIMAL         330
#define KEY_KP_DIVIDE          331
#define KEY_KP_MULTIPLY        332
#define KEY_KP_SUBTRACT        333
#define KEY_KP_ADD             334
#define KEY_KP_ENTER           335
#define KEY_KP_EQUAL           336
#define KEY_LEFT_SHIFT         340
#define KEY_LEFT_CONTROL       341
#define KEY_LEFT_ALT           342
#define KEY_LEFT_SUPER         343
#define KEY_RIGHT_SHIFT        344
#define KEY_RIGHT_CONTROL      345
#define KEY_RIGHT_ALT          346
#define KEY_RIGHT_SUPER        347
#define KEY_MENU               348
#define KEY_LAST               KEY_MENU

#endif // PUNITY_H