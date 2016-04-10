/** 
 * Copyright (c) 2016 martincohen
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */ 

/**
 * Version 1.2
 * - Replaced usage of `_` prefixes with `PUNP_` prefixes to not violate reserved identifiers.
 * Version 1.1
 * - Fixed timing issues and frame counting.
 * - Fixed KEY_* constants being not correct for Windows platform.
 * - Added draw_rect().
 * Version 1.0
 * - Initial release
 */

#ifndef PUNITY_H
#define PUNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifndef NO_CONFIG
#include "config.h"
#endif

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
#define clamp(x, a, b)  (maximum(a, minimum(x, b)))

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


// Draws a filled rectangle to the canvas.
void rect_draw(Rect rect, u8 color);
// Draws a frame of a rectangle to the canvas.
void frame_draw(Rect rect, u8 color);

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
	PerfSpan perf_blit_cvt;
	PerfSpan perf_blit_gdi;

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

#define KEY_UNKNOWN            -1
#define KEY_INVALID            -2

#define KEY_LBUTTON	            1
#define KEY_RBUTTON	            2
#define KEY_CANCEL              3
#define KEY_MBUTTON             4

#define KEY_BACK                8
#define KEY_TAB                 9
#define KEY_CLEAR               12
#define KEY_RETURN              13
#define KEY_SHIFT               16
#define KEY_CONTROL             17
#define KEY_MENU                18
#define KEY_PAUSE               19
#define KEY_CAPITAL             20
#define KEY_KANA                0x15
#define KEY_HANGEUL             0x15
#define KEY_HANGUL              0x15
#define KEY_JUNJA               0x17
#define KEY_FINAL               0x18
#define KEY_HANJA               0x19
#define KEY_KANJI               0x19
#define KEY_ESCAPE              0x1B
#define KEY_CONVERT             0x1C
#define KEY_NONCONVERT          0x1D
#define KEY_ACCEPT              0x1E
#define KEY_MODECHANGE          0x1F
#define KEY_SPACE               32
#define KEY_PRIOR               33
#define KEY_NEXT                34
#define KEY_END                 35
#define KEY_HOME                36
#define KEY_LEFT                37
#define KEY_UP                  38
#define KEY_RIGHT               39
#define KEY_DOWN                40
#define KEY_SELECT              41
#define KEY_PRINT               42
#define KEY_EXEC                43
#define KEY_SNAPSHOT            44
#define KEY_INSERT              45
#define KEY_DELETE              46
#define KEY_HELP                47
#define KEY_LWIN                0x5B
#define KEY_RWIN                0x5C
#define KEY_APPS                0x5D
#define KEY_SLEEP               0x5F
#define KEY_NUMPAD0             0x60
#define KEY_NUMPAD1             0x61
#define KEY_NUMPAD2             0x62
#define KEY_NUMPAD3             0x63
#define KEY_NUMPAD4             0x64
#define KEY_NUMPAD5             0x65
#define KEY_NUMPAD6             0x66
#define KEY_NUMPAD7             0x67
#define KEY_NUMPAD8             0x68
#define KEY_NUMPAD9             0x69
#define KEY_MULTIPLY            0x6A
#define KEY_ADD                 0x6B
#define KEY_SEPARATOR           0x6C
#define KEY_SUBTRACT            0x6D
#define KEY_DECIMAL             0x6E
#define KEY_DIVIDE              0x6F
#define KEY_F1                  0x70
#define KEY_F2                  0x71
#define KEY_F3                  0x72
#define KEY_F4                  0x73
#define KEY_F5                  0x74
#define KEY_F6                  0x75
#define KEY_F7                  0x76
#define KEY_F8                  0x77
#define KEY_F9                  0x78
#define KEY_F10                 0x79
#define KEY_F11                 0x7A
#define KEY_F12                 0x7B
#define KEY_F13                 0x7C
#define KEY_F14                 0x7D
#define KEY_F15                 0x7E
#define KEY_F16                 0x7F
#define KEY_F17                 0x80
#define KEY_F18                 0x81
#define KEY_F19                 0x82
#define KEY_F20                 0x83
#define KEY_F21                 0x84
#define KEY_F22                 0x85
#define KEY_F23                 0x86
#define KEY_F24                 0x87
#define KEY_NUMLOCK             0x90
#define KEY_SCROLL              0x91
#define KEY_LSHIFT              0xA0
#define KEY_RSHIFT              0xA1
#define KEY_LCONTROL            0xA2
#define KEY_RCONTROL            0xA3
#define KEY_LMENU               0xA4
#define KEY_RMENU               0xA5

#define KEY_SPACE               32
#define KEY_APOSTROPHE          39  /* ' */
#define KEY_COMMA               44  /* , */
#define KEY_MINUS               45  /* - */
#define KEY_PERIOD              46  /* . */
#define KEY_SLASH               47  /* / */
#define KEY_0                   48
#define KEY_1                   49
#define KEY_2                   50
#define KEY_3                   51
#define KEY_4                   52
#define KEY_5                   53
#define KEY_6                   54
#define KEY_7                   55
#define KEY_8                   56
#define KEY_9                   57
#define KEY_SEMICOLON           59  /* ; */
#define KEY_EQUAL               61  /* = */
#define KEY_A                   65
#define KEY_B                   66
#define KEY_C                   67
#define KEY_D                   68
#define KEY_E                   69
#define KEY_F                   70
#define KEY_G                   71
#define KEY_H                   72
#define KEY_I                   73
#define KEY_J                   74
#define KEY_K                   75
#define KEY_L                   76
#define KEY_M                   77
#define KEY_N                   78
#define KEY_O                   79
#define KEY_P                   80
#define KEY_Q                   81
#define KEY_R                   82
#define KEY_S                   83
#define KEY_T                   84
#define KEY_U                   85
#define KEY_V                   86
#define KEY_W                   87
#define KEY_X                   88
#define KEY_Y                   89
#define KEY_Z                   90
#define KEY_LEFT_BRACKET        91  /* [ */
#define KEY_BACKSLASH           92  /* \ */
#define KEY_RIGHT_BRACKET       93  /* ] */
#define KEY_GRAVE_ACCENT        96  /* ` */

#endif // PUNITY_H