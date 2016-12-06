#ifndef PUNITY_H
#define PUNITY_H

/** 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015-2016 Martin Cohen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * Version 2.0
 *
 * - Switched to single file `punity.h`.
 * - Removed the need for `config.h`.
 *
 * - Added GIF recording.
 *   - Depends on `lib/gifw.h`.
 *   - Forced to 30fps. (even if Punity runs on 60fps)
 *   - Call `record_begin()` to start recording.
 *   - Call `record_end()` to end recording and save the recording to `record.gif`.
 *     - Saving the `gif` might take longer time so the frame in which the record_end() is called
 *       will take longer than 30 or 60fps (depending on your settings).
 *
 * - Added SDL runtime. (still work-in-progress)
 *   - Depends on `lib/punity-sdl.c`.
 *   - `SDL2.dll` has to be distributed with the executable.
 *   - Build with `build sdl`
 *    - Reorganized `main()`, better platform code separation.
 *
 * - Added entity and collision system.
 *   - Customizable `SceneEntity` struct with `PUN_SCENE_ENTITY_CUSTOM`.
 *   - Uses `SpatialHash` for entities.
 *   - Uses fixed tile map for static colliders.
 *     - Much faster than adding each tile to `SpatialHash`.
 *   - Integer-only without need to fiddle with float epsilons.
 *   - Uses `Deque` to store "unlimited amount" of entities.
 *   - Use `entity_add` / `entity_remove` to add or remove entities.
 *   - Use `entity_move_x`, `entity_move_y` to move your entities.
 *
 * - Reworked `build.bat`.
 *    - Now able to build different targets than `main.c` (i.e. build examples/example-platformer)
 *    - Arguments can be in any order.
 *
 * - Added PUNITY_OPENGL_30FPS to disable forcing framerate to 30fps.
 * - Removed DirectX support (moved to a separate module that will be optional in the future).
 *   - This change has been made to keep the runtime layer in `punity.h` file as simple as possible.
 * - Changed `init` signature to return `int` to be able to exit in case of error.
 * - Changed `Bank` now reserves given capacity and commits when needed.
 * - Changed `file_read` now takes optional `bank` pointer to store the file, if it's 0 then it'll use malloc()
 * - Changed COLOR_CHANNELS to PUNITY_COLOR_CHANNELS
 * - Added `configure` callback now added to do runtime configuration for some options (see CoreConfig)
 * - Added `window_title(title)` to set the title of the window dynamically.
 * - Added `window_fullscreen_toggle()` to maximize the window to fullscreen and back.
 * - Added `Deque` for dynamic allocations.
 * - Added `pixel_draw`
 * - Added `line_draw`
 * - Added `color_set` to set color at given index in the palette.
 * - Added `frame_draw` to draw rectangle outlines.
 * - Added `CORE->time`.
 * - Moved `color_lerp` is now in header file.
 * - Added `CORE->key_text` and `CORE->key_text_length` to be able to work with text input.
 * - Fixed bug with drawing bitmaps higher than the screen.
 * - Removed `PUN_COLOR_BLACK` and `PUN_COLOR_WHITE`.
 *
 * Version 1.6
 * - Version actually used for Ludum Dare and Low-Rez jams.
 * - "Cherchez" has been made with this version: https://martincohen.itch.io/cherchez
 * - All the configuration macros are now prefixed with PUN_
 * - Fixed bug in sound mixing.
 * - All rendering properties are now stored in dedicated `Canvas` struct.
 * - Added simple randomization functions (see the documentation).
 * - Added `file_write` function to accompany `file_read`.
 * - Added basic `V2f` struct and functions.
 * - Added vertical flipping for `bitmap_draw` function.
 * - Reworked `bitmap_draw` function.
 * - Added drawing list support (see the documentation).
 * - Added `*_push` functions for pushing draw operations to draw list (see the documentation)
 *
 * Version 1.5
 * - Renamed configuration macros to use PUN_* prefix
 * - PUN_MAIN can now be used to not define the main entry function.
 * - Forced PUN_COLOR_BLACK to 1 and PUN_COLOR_WHITE to 2.
 * - Changed bitmap_draw signature to have Bitmap* at the beginning.
 * - Changed text_draw signature to have const char* at the beginning.
 * - Translation direction changed in bitmap_draw() and text_draw().
 *
 * Version 1.4
 * - Fixed audio clipping problems by providing a soft-clip.
 * - Added master and per-sound volume controls.
 *
 * Version 1.3
 * - Fixed RGB in bitmap loader on Windows.
 *
 * Version 1.2
 * - Replaced usage of `_` prefixes with `PUNP_` prefixes to not violate reserved identifiers.
 *
 * Version 1.1
 * - Fixed timing issues and frame counting.
 * - Fixed KEY_* constants being not correct for Windows platform.
 * - Added draw_rect().
 *
 * Version 1.0
 * - Initial release
 */


#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include <stdint.h>
#include <stdbool.h>
// #include <math.h>
// #include <assert.h>
// #include <stdarg.h>
// #include <limits.h>

#if _DEBUG
#define PUNITY_DEBUG 1
#else
#define PUNITY_DEBUG 0
#endif

// On Windows, enables external console window for printf output.
//
#ifndef PUNITY_CONSOLE
#define PUNITY_CONSOLE 0
#endif

// Only implements the Punity's functions and types.
// Useful for making tools that use Punity's library.
//
#ifndef PUNITY_LIB
#define PUNITY_LIB 0
#endif

// This makes all "LOG" calls to output into Visual Studio's Ouput
// through OutputDebugStringA() API.
//
#ifndef PUNITY_DEBUG_MSVC
#define PUNITY_DEBUG_MSVC 0
#endif

// Enables/disables SIMD acceleration for bitmap drawing.
// Speeds up drawing from 2x up to 16x.
// Larger bitmap, more acceleration.
//
#ifndef PUNITY_SIMD
#define PUNITY_SIMD 1
#endif

// Enables/disables OpenGL blitting.
// Minimal OpenGL loader provided.
// Thanks to @ApoorvaJ.
//
#ifndef PUNITY_OPENGL
#define PUNITY_OPENGL 1
#endif

// Enables/disables forcing of 30fps when OpenGL is used.
//
#ifndef PUNITY_OPENGL_30FPS
#define PUNITY_OPENGL_30FPS 1
#endif

// Enables/disables GIF recorder.
// Use F11 to begin GIF recording, and F11 to stop and output the
// GIF file to disk.
// Customize the shortcut with PUNITY_FEATURE_RECORDER_KEY macro.
// Uses lib/gifw.h.
//
#ifndef PUNITY_FEATURE_RECORDER
#define PUNITY_FEATURE_RECORDER 1
#endif

// Can be set to any KEY_* constant, it is automatically used for recording.
// If set to 0, the automatic key binding is removed so you'll have to
// implement it yourself.
//
#ifndef PUNITY_FEATURE_RECORDER_KEY
#define PUNITY_FEATURE_RECORDER_KEY KEY_F11
#endif

// Scale used for recording.
//
#ifndef PUNITY_FEATURE_RECORDER_SCALE
#define PUNITY_FEATURE_RECORDER_SCALE 2
#endif

// Maximum number of bytes available in `CORE->stack` bank.
//
#ifndef PUNITY_STACK_CAPACITY
#define PUNITY_STACK_CAPACITY megabytes(16)
#endif

// Maximum number of bytes available in `CORE->storage` bank.
//
#ifndef PUNITY_STORAGE_CAPACITY
#define PUNITY_STORAGE_CAPACITY megabytes(16)
#endif

// When 1, `CORE->key_text` is will be a UTF-32 string.
// However for most of use cases, `char` would do.
//
#ifndef PUNITY_TEXT_UNICODE
#define PUNITY_TEXT_UNICODE 0
#endif

// How many items to reserve for the draw list.
// It's not very important to get this one right,
// system allocates more space for next frame,
// if current frame's allocation was not enough.
//
#ifndef PUNITY_DRAW_LIST_RESERVE
#define PUNITY_DRAW_LIST_RESERVE 4096
#endif

// Enables integration with `stb_image.h` library.
// Allows for loading common image formats.
//
#ifndef PUNITY_USE_STB_IMAGE
#define PUNITY_USE_STB_IMAGE 1
#endif

// Enables integration with `stb_vorbis.c` library.
// Allows for using `.ogg` file format for audio.
//
#ifndef PUNITY_USE_STB_VORBIS
#define PUNITY_USE_STB_VORBIS 1
#endif

// Sample rate used internally in Punity.
// This rate is requested from the system (it's the most common one).
// All audio is resampled to this frequency.
//
#ifndef PUNITY_SOUND_SAMPLE_RATE
#define PUNITY_SOUND_SAMPLE_RATE 48000
#endif

// FileWatcher API.
//
#ifndef PUNITY_FEATURE_FILEWATCHER
#define PUNITY_FEATURE_FILEWATCHER 0
#endif

// World API.
//
#ifndef PUNITY_FEATURE_WORLD
#define PUNITY_FEATURE_WORLD 0
#endif

// Platform specific color channel ordering.
// As more platforms are available, this will be set automatically per platform.
//
#define PUNITY_COLOR_CHANNELS b, g, r, a

typedef unsigned int uint;
typedef uint8_t   u8;
typedef int8_t    i8;
typedef uint16_t  u16;
typedef int16_t   i16;
typedef uint32_t  u32;
typedef int32_t   i32;
typedef uint64_t  u64;
typedef int64_t   i64;
typedef float     f32;
typedef double    f64;
typedef u32       b32;
typedef size_t    usize;
typedef ptrdiff_t isize;

#define unused(x) (void)x

#if defined(_MSC_VER)
#define PUN_ALIGN_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define PUN_ALIGN_(x) __attribute__ ((aligned(x)))
#endif
#endif

#define align_to(value, N) ((value + (N-1)) & ~(N-1))
#define ceil_div(n, a) (((n) + (a-1))/(a))
#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)

#define maximum(a, b) ((a) > (b) ? (a) : (b))
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#define clamp(x, a, b)  (maximum(a, minimum(x, b)))
#define array_count(a) (sizeof(a) / sizeof((a)[0]))
#define swap_t(T, a, b) do { T tmp__ = a; a = b; b = tmp__; } while (0)

#define lerp(v0, v1, t) ((1-(t))*(v0) + (t)*(v1))

// Returns 1 or -1.
extern inline i32 i32_sign(i32 i);
// Returns absolute value.
extern inline i32 i32_abs(i32 i);
// Returns 1 if `i` is positive, 0 otherwise.
extern inline i32 i32_positive(i32 i);
// Returns 1 if `i` is negative, 0 otherwise.
extern inline i32 i32_negative(i32 i);

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

//
// Forwards
//

typedef struct DrawListItem_ DrawListItem;

//
// Utility
//

u32 rand_u(u32 *x);
f32 rand_f(u32 *x);
f32 rand_fr(u32 *x, f32 min, f32 max);
i32 rand_ir(u32 *x, i32 min, i32 max);

typedef struct
{
    i32 x, y;
}
V2;

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
    u8 *cap;
}
Bank;

void *virtual_reserve(void* ptr, u32 size);
void *virtual_commit(void *ptr, u32 size);
void virtual_decommit(void *ptr, u32 size);
void virtual_free(void *ptr, u32 size);
void *virtual_alloc(void* ptr, u32 size);

void bank_init(Bank *bank, u32 capacity);
void bank_clear(Bank *bank);
void bank_zero(Bank *bank);
void *bank_push(Bank *bank, u32 size);
void bank_pop(Bank *bank, void *ptr);
void bank_free(Bank *bank);

#define bank_push_t(Bank, Type, Count) \
    ((Type*)bank_push(Bank, (Count) * sizeof(Type)))


typedef struct
{
    Bank state;
    Bank *bank;
}
BankState;

BankState bank_begin(Bank *bank);
void bank_end(BankState *state);

//
// Deque
//

typedef struct DequeBlock_ DequeBlock;

typedef struct DequeBlock_
{
    uint8_t *begin;
    uint8_t *end;
    uint8_t *it;
    DequeBlock *next;
}
DequeBlock;

typedef struct Deque_
{
    DequeBlock *first;
    DequeBlock *last;
    DequeBlock *pool;
    size_t count;
    size_t block_size;

    // Total number of bytes in deque.
    size_t size;
}
Deque;

#define DEQUE_WALK(name) int name(Deque *deque, uint8_t *begin, uint8_t *end, int progress, void *user)
typedef DEQUE_WALK(DequeWalkF);

// Initializes the deque.
void deque_init(Deque *deque, size_t block_size);
// Clears the deque, keeps the allocated memory for reuse.
void deque_clear(Deque *deque);
// Frees all the resources allocated for the deque.
// Puts deque back into state as if it was just initialized.
// Keeps the same block_size.
void deque_free(Deque *deque);
// Allocates a new block and returns pointer to it's start.
void * deque_block_push(Deque *deque);
// Appends `data` passed to the deque.
// The data is split to fill the blocks.
// To add data without splitting see `deque_push`.
// Returns pointer to the first byte where the data was inserted.
void * deque_append(Deque *deque, void *data, size_t size);
// Returns a pointer to a continuous block of deque memory to fit `size` bytes in.
// The `size` must be currently less or equal to `deque->block_size`.
// TODO: Allow `size` to be not dependent on deque->block_size.
//       - Allocate block big enough to fit the data in.
void * deque_push(Deque *deque, size_t size);

#define deque_append_t(Deque, Type, Ptr) \
    ((Type*)deque_append((Deque), (Ptr), sizeof(Type)))

#define deque_push_t(Deque, Type) \
    ((Type*)deque_push((Deque), sizeof(Type)))

#define deque_block_push_t(Deque, Type) \
    ((Type*)deque_block_push((Deque)))

#define DEQUE_PUSH_SCALAR_DECLARE(Name, Type) \
    Type *deque_push_scalar_##Name(Deque *deque, Type v)
#define DEQUE_PUSH_SCALAR_IMPLEMENT(Name, Type) \
    DEQUE_PUSH_SCALAR_DECLARE(Name, Type) \
    { return deque_append_t(deque, Type, &v); }

DEQUE_PUSH_SCALAR_DECLARE(u8, uint8_t);
DEQUE_PUSH_SCALAR_DECLARE(i8, int8_t);
DEQUE_PUSH_SCALAR_DECLARE(i16, int16_t);
DEQUE_PUSH_SCALAR_DECLARE(u16, uint16_t);
DEQUE_PUSH_SCALAR_DECLARE(i32, int32_t);
DEQUE_PUSH_SCALAR_DECLARE(u32, uint32_t);

//
// Strings
//

char *string_push_0(char *dest, const char *src);
char *string_push(char *dest, const char *src_it, const char *src_end);
size_t string_concat(char *buffer, size_t size, const char *str, ...);
extern inline char *string_copy_0(char *dest, size_t size, const char *str);

//
// File I/O
//

void *file_read(Bank *bank, const char *path, size_t *size);
bool file_write(const char *path, void *ptr, size_t size);


//
// Path
//

#define path_is_slash(Ch) ((Ch) == '/' || (Ch) == '\\')

#define PUN_PATH_MAX (1024)
#if PUN_PLATFORM_WINDOWS
#define PUN_PATH_SEPARATOR ('\\')
#else
#define PUN_PATH_SEPARATOR ('/')
#endif

// Normalizes zero-terminated path to common format.
char *path_normalize_0(char *path);
// Normalizes path to common format.
char *path_normalize(char *it, char *end);
// Concats multiple path elements.
// Automatically inserts slashes between the elements when needed.
// IMPORTANT: The last parameter should always be 0.
uint32_t path_concat(char *buffer, uint32_t size, const char *arg1, ...);


// Returns extension of the path (pointer to the string after last dot).
const char *path_extension_0(const char *path);
const char *path_extension(const char *it, const char *end);
// Returns name from the path (pointer to the string after last slash).
const char *path_file_name_0(const char *path);
const char *path_file_name(const char *it, const char *end);

//
// Math
//

typedef struct
{
    f32 x;
    f32 y;
}
V2f;

extern inline V2f v2f_make(f32 x, f32 y);
extern inline f32 v2f_length(V2f v);
extern inline V2f v2f_normalize(V2f v);
extern inline f32 v2f_angle(V2f v);
extern inline V2f v2f_minus(V2f a, V2f b);

#define v2_inner(ax, ay, bx, by) \
    ((ax)*(bx) + (ay)*(by))

//
// Graphics
//

enum
{
    Edge_Left       = 1 << 0,
    Edge_Top        = 1 << 1,
    Edge_Right      = 1 << 2,
    Edge_Bottom     = 1 << 3,
    Edge_All        = Edge_Left | Edge_Top | Edge_Right | Edge_Bottom,
};

#define PUN_COLOR_TRANSPARENT 0

typedef union
{
    struct {
        u8 PUNITY_COLOR_CHANNELS;
    };
    u32 rgba;
}
Color;

extern inline Color color_make(u8 r, u8 g, u8 b, u8 a);
extern inline Color color_lerp(Color from, Color to, f32 t);

typedef struct
{
    int begin;
    int end;
    int it;
}
PaletteRange;

#ifndef PUN_PALETTE_RANGES_COUNT
#define PUN_PALETTE_RANGES_COUNT (16)
#endif

typedef struct
{
    Color colors[256];
    PaletteRange ranges[PUN_PALETTE_RANGES_COUNT];
    int ranges_count;
#ifdef PUN_PALETTE_RANGE_CUSTOM
    PUN_PALETTE_RANGE_CUSTOM
#endif
}
Palette;

void palette_init(Palette *palette);
void palette_load(Palette *palette, const char *path, int offset);
void palette_load_resource(Palette *palette, const char *resource_name, int offset);
int palette_range_add(Palette *palette, int begin, int end, int it);
extern inline u8 palette_color_set(Palette *palette, u8 index, Color color);
extern inline int palette_color_add(Palette *palette, Color color, int range);
extern inline int palette_color_acquire(Palette *palette, Color color, int range);


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

#define rect_width(r) \
    ((r)->max_x - (r)->min_x)

#define rect_height(r) \
    ((r)->max_y - (r)->min_y)

extern inline Rect rect_make(i32 min_x, i32 min_y, i32 max_x, i32 max_y);
extern inline Rect rect_make_size(i32 x, i32 y, i32 w, i32 h);
extern inline Rect rect_make_centered(i32 x, i32 y, i32 w, i32 h);
extern inline Rect rect_clip(Rect rect, Rect clip);
extern inline bool rect_contains_point(Rect rect, i32 x, i32 y);
extern inline bool rect_overlaps(Rect rect_a, Rect rect_b);
extern inline void rect_center(Rect rect, i32 *x, i32 *y);

typedef struct
{
    i32 width;
    i32 height;
    i32 pitch;
    u8 *pixels;
    i32 palette_range;
    i32 tile_width;
    i32 tile_height;
#ifdef PUN_BITMAP_CUSTOM
    PUN_BITMAP_CUSTOM
#endif
}
Bitmap;

typedef struct
{
    Bitmap bitmap;
    i32 char_width;
    i32 char_height;
#ifdef PUN_FONT_CUSTOM
    PUN_FONT_CUSTOM
#endif
}
Font;

enum {
    DrawFlags_None  = 0,
    DrawFlags_FlipH = 1 << 0,
    DrawFlags_FlipV = 1 << 1,
    DrawFlags_Mask  = 1 << 2,
};

typedef struct
{
    Bitmap *bitmap;
    i32 translate_x;
    i32 translate_y;
    Rect clip;
    Font *font;
    u32 flags;
    u8 mask;
#ifdef PUN_CANVAS_CUSTOM
    PUN_CANVAS_CUSTOM
#endif
}
Canvas;

#define PUN_BITMAP_MASK 0
#define PUN_BITMAP_8    1
#define PUN_BITMAP_32   4

void camera_reset();
void camera_set(i32 x, i32 y, Rect *clip);
i32 camera_x();
i32 camera_y();

// Initializes the bitmap struct and copies the data from pixels to the CORE->storage.
// If pixels is 0, the bitmap's pixel data are only allocated (not initialized)
// If pixels is not 0 then the `type` is used as follows:
//
// - BITMAP_32, it'll convert it to paletted image by adding the unknown colors to the palette.
// - BITMAP_8,  the data are copied as they are.
//
void bitmap_init(Bitmap *bitmap, i32 width, i32 height, void *pixels, int type, int palette_range);
void bitmap_clear(Bitmap *bitmap, u8 color);

// Loads bitmap from an image file.
// This only works if USE_STB_IMAGE is defined.
#if PUNITY_USE_STB_IMAGE
void bitmap_load(Bitmap *bitmap, const char *path, int palette_range);
void bitmap_load_resource(Bitmap *bitmap, const char *resource_name, int palette_range);
void bitmap_load_resource_ex(Bank *bank, Bitmap *bitmap, const char *resource_name, int palette_range);
#endif

#if PUNITY_USE_STB_IMAGE
void font_load_resource(Font *bitmap, const char *resource_name, i32 char_width, i32 char_height);
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

// Draws a single pixel, sub-optimal, don't use, please.
void pixel_draw(i32 x, i32 y, u8 color);

// Draws a line.
void line_draw(i32 x1, i32 y1, i32 x2, i32 y2, u8 color);

// Draws a filled rectangle to the canvas.
void rect_draw(Rect rect, u8 color);

// Draws rectangle edges specified by `frame_edges` (Edge_* constants)
// with a `frame_color` and `fill_color`.
void frame_draw(Rect r, u8 frame_color, int frame_edges, u8 fill_color);

// Draws a bitmap to the canvas.
void bitmap_draw_single_(Bitmap *bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect);

#if PUNITY_SIMD
void bitmap_draw_simd_(Bitmap *bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect);
#define bitmap_draw bitmap_draw_simd_
#else
#define bitmap_draw bitmap_draw_single_
#endif

// Returns a tile rectangle in the bitmap based on `index`.
Rect tile_get(Bitmap *bitmap, i32 index);

// Draws a tile from bitmap (utilizing Bitmap's tile_width/tile_height).
void tile_draw(Bitmap *bitmap, i32 x, i32 y, i32 index);

// Copies bitmap from `source` to `destination`
void bitmap_copy(Bitmap *destination, Bitmap *source);

// Calculates width and height of the text using current font.
V2 text_measure(const char *text);
// Draws text to the canvas.
// Fails if CORE->font is not set, as it's using it draw the text.
void text_draw(const char *text, i32 x, i32 y, u8 color);

typedef struct
{
    u8 fg;
    u8 bg;
}
TextAttr;

V2 text_draw_attr(const char *text, i32 x, i32 y, TextAttr *attrs, size_t length);


//
// DrawList
//

#define DRAWLIST_CALLBACK(name) void name(void *data, size_t data_size)
typedef DRAWLIST_CALLBACK(DrawListCallbackF);

typedef enum
{
    DrawListItemType_Callback,
    DrawListItemType_Line,
    DrawListItemType_Rect,
    DrawListItemType_Frame,
    DrawListItemType_Text,
    DrawListItemType_BitmapFull,
    DrawListItemType_BitmapPartial
}
DrawListItemType;

struct DrawListItem_
{
    u32 type;
    u32 key;
    Canvas canvas;

    union
    {
        struct {
            i32 x1, y1, x2, y2;
            u8 color;
        } line;

        struct {
            Rect rect;
            u8 color;
        } rect;
        
        struct {
            char *text;
            i32 x;
            i32 y;
            u8 color;
        } text;

        struct {
            Bitmap *bitmap;
            i32 x;
            i32 y;
            i32 pivot_x;
            i32 pivot_y;
            Rect bitmap_rect;
        } bitmap;
        
        struct {
            void *data;
            size_t data_size;
            DrawListCallbackF *callback;
        } callback;
    };

#ifdef PUN_DRAWLISTITEM_CUSTOM
    PUN_DRAWLISTITEM_CUSTOM
#endif
};

typedef struct
{
    // TODO: Potentially we don't need both of these, storage would do.
    DrawListItem *items_storage;
    DrawListItem **items;
    size_t items_count;
    size_t items_reserve;
    size_t items_additional;
    f64 perf;
}
DrawList;

void drawlist_init(DrawList *list, size_t reserve);

void drawlist_begin(DrawList *list);
void drawlist_end(DrawList *list);
void drawlist_clear(DrawList *list);

DrawListItem *drawlist_callback_push_blob(DrawList *list, DrawListCallbackF *callback, void *data, size_t data_size, i32 z);
DrawListItem *drawlist_callback_push_ptr(DrawList *list, DrawListCallbackF *callback, void *ptr, i32 z);
DrawListItem *bitmap_draw_push(Bitmap *bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect, i32 z);
DrawListItem *text_draw_push(const char *text, i32 x, i32 y, u8 color, i32 z);
DrawListItem *frame_draw_push(Rect r, u8 frame_color, int frame_edges, u8 fill_color, i32 z);
DrawListItem *rect_draw_push(Rect rect, u8 color, i32 z);
DrawListItem *line_draw_push(i32 x1, i32 y1, i32 x2, i32 y2, u8 color, i32 z);
DrawListItem *tile_draw_push(Bitmap *bitmap, i32 x, i32 y, i32 index, i32 z);

//
// Debug
//

void debug_palette(i32 x, i32 y, i32 z);

//
// Sound
//

enum
{
    SoundFlag_Loop        = 1 << 1,
    SoundFlag_FadeOut     = 1 << 2,
    SoundFlag_FadeIn      = 1 << 3,
};

typedef struct
{
    char *name;
    u32 flags;
    f32 volume;
    i32 channels;
    u32 rate;
    i16 *samples;
    // Samples per channel.
    size_t samples_count;
    i32 sources_count;
}
Sound;

void sound_play(Sound *sound);

#if PUNITY_USE_STB_VORBIS
void sound_load(Sound *sound, const char *path);
void sound_load_resource(Sound *sound, const char *resource_name);
#endif

//
// Windowing
//

void window_title(const char *title);
bool window_is_fullscreen();
void window_fullscreen_toggle();

//
// Resources
//

void *resource_get(const char *name, size_t *size);

//
// GIF recording.
//

#if PUNITY_FEATURE_RECORDER

#include <gifw.h>

#define PUN_RECORDER_FRAMES_PER_BLOCK (256)

// A single frame.
typedef struct RecorderFrame_
{
    uint8_t *pixels;
    GIFWColorTable color_table;
}
RecorderFrame;

// Block of PUN_RECORDER_FRAMES_PER_BLOCK frames.
typedef struct RecorderFrameBlock_
{
    uint8_t *pixels;
    RecorderFrame frames[PUN_RECORDER_FRAMES_PER_BLOCK];
    size_t frames_count;
}
RecorderFrameBlock;

typedef struct Recorder_ {
    bool active;
    Deque frames;
    GIFW gif;
    size_t frames_count;
} Recorder;

#endif // PUNITY_FEATURE_RECORDER

void record_begin();
void record_end();
void record_toggle();

//
// File watcher.
//

#if PUNITY_FEATURE_FILEWATCHER

#include <sys/stat.h>
#include <time.h>

#define FILEWATCHER_CALLBACK(name) int name(const char *path, int path_length, void *extra)
typedef FILEWATCHER_CALLBACK(FileWatcherCallbackF);

typedef struct FileWatcherEntry_ {
    char *path;
    int path_length;
    time_t time;
    FileWatcherCallbackF *f;
    size_t extra_size;
}
FileWatcherEntry;

#define filewatcherentry_size(Entry) \
    (sizeof(FileWatcherEntry) + (Entry)->path_length + 1 + (Entry)->extra_size)

typedef struct FileWatcher_ {
    Deque files;
    size_t count;
    size_t count_inactive;
}
FileWatcher;

#endif // PUNITY_FEATURE_FILEWATCHER

#if PUNITY_FEATURE_WORLD

//
// State
//

typedef struct 
{
    i32 current;
    i32 next;
    i32 frame;
    i32 timer;
}
State;

void state_init(State *S, i32 state);
void state_step(State *S);

//
// Spatial Hash
//

typedef struct SpatialCell_
{
    void *items[16];
    u8 items_count;
    i32 x, y;
    struct SpatialCell_ *next;
}
SpatialCell;

typedef struct SpatialHash_
{
    Deque cells;
    size_t buckets_count;
    SpatialCell **buckets;
    Rect rect;

    SpatialCell *pool;
}
SpatialHash;

#ifndef spatialhash_hash
#define spatialhash_hash(X, Y) ((u32)(7*(X) + 3*(Y)))
#endif

#ifndef spatialhash_bucket
#define spatialhash_bucket(H, Hash) ((size_t)((Hash) % ((H)->buckets_count)))
#endif


// Initializes the spatial hash with given `buckets_count`.
void spatialhash_init(SpatialHash *H, size_t buckets_count);

// Frees the spatial hash.
void spatialhash_free(SpatialHash *H);

// Adds the `item` to reside on given rectangle.
// Note that it's not defined what happens when you add the same item multiple times.
// Use `spatialhash_remove` to remove it first or `spatialhash_update` to change the assignment.
void spatialhash_add(SpatialHash *H, Rect range, void *item);

// Removes `item` from the given rect range.
// If the item is not there, nothing is removed, you just wasted CPU time.
void spatialhash_remove(SpatialHash *H, Rect range, void *item);

// Changes the rectangle of the item from `old_range` to `new_range`.
void spatialhash_update(SpatialHash *H, void *item, Rect old_range, Rect new_range);

// Returns a cell at given `x`, `y`. If no cell is available at that location, returns NULL.
// Note that there can be multiple succeeding cells for the same `x` and `y`.
//
// You can iterate over the cells and items like this:
/*
    cell = spatialhash_get_cell(H, 13, 37);
    while (cell & (cell->x == 13 && cell->y == 37)) {
        for (int i = 0; i != cell->items_count; ++i) {
            Do something with `cell->items[i]`.
        }
    }
*/
SpatialCell *spatialhash_get_cell(SpatialHash *H, int x, int y);

//
// Collider
//

typedef struct SceneTile_ SceneTile;
typedef struct SceneEntity_ SceneEntity;

typedef struct SceneEntity_ {
    Rect box;
    i32 flags;
    f32 rx, ry;
    i32 layer;
    void *next;
#ifdef PUN_SCENE_ENTITY_CUSTOM
    PUN_SCENE_ENTITY_CUSTOM
#endif
} SceneEntity;

typedef struct SceneTile_
{
    u8 flags;
    i32 layer;
#ifdef PUN_SCENE_TILE_CUSTOM
    PUN_SCENE_TILE_CUSTOM
#endif
} SceneTile;

enum {
    SceneItem_None = 0,
    SceneItem_Entity,
    SceneItem_Tile,
};

typedef struct SceneItem_ {
    i32 type;
    union {
        SceneEntity *entity;
        SceneTile *tile;
        void *ptr;
    };
} SceneItem;

typedef struct
{
    i32 type;
    // Bank *bank;
    SceneEntity *A;
    SceneItem B;
    i32 mx, my;
    Rect box;
}
Collision;

//
// World
//

enum {
    // Bits 1-4 are reserved for ColliderFlag_Edge*.
    EntityFlag_Tested  = 1 << 5,
    EntityFlag_Ignored = 1 << 6,
    EntityFlag_Removed = 1 << 7,
    EntityFlag_Tile    = 1 << 8,
    EntityFlag_CustomShift = 9,
};

typedef struct Scene_
{
    int initialized;

    // TODO: Rename to entities.
    Deque deque_entities;

    SceneEntity *entities;
    SceneEntity *entities_pool;
    i32 entities_count;
    i32 cell_size;
    SpatialHash hash;

    Collision collision;

    i32 tile_size;
    Rect tile_rect;
    SceneTile *tiles;
}
Scene;

// Initializes the world.
void scene_init(Scene *scene, i32 tile_size, i32 tilemap_width, i32 tilemap_height, i32 cell_size);
void scene_clear(Scene *scene);
#define SCENE_FOREACH_CALLBACK(name) bool name(Scene *scene, Rect *cast_box, Rect *item_box, i32 item_flags, SceneItem *item, void *data)
typedef SCENE_FOREACH_CALLBACK(SceneForEachCallbackF);
bool scene_foreach(Scene *scene, Rect rect, SceneForEachCallbackF *callback, void *data, i32 layer);
void scene_debug_tilemap(Scene *scene, u8 color, i32 z);
void scene_debug_cells(Scene *scene, u8 color, i32 z);

extern inline SceneTile *scene_tile(Scene *scene, i32 x, i32 y);

SceneEntity *scene_entity_add(Scene *scene, Rect box, i32 layer);
void scene_entity_remove(Scene *scene, SceneEntity *entity);

bool scene_entity_cast_x(Scene *scene, SceneEntity *entity, f32 dx, Collision *C);
bool scene_entity_cast_y(Scene *scene, SceneEntity *entity, f32 dy, Collision *C);
bool scene_entity_move_x(Scene *scene, SceneEntity *entity, f32 dx, Collision *C);
bool scene_entity_move_y(Scene *scene, SceneEntity *entity, f32 dy, Collision *C);

#endif // PUNITY_FEATURE_WORLD

//
// Core
//

typedef struct Window_
{
    int width;
    int height;
    f32 scale;

    f32 viewport_min_x;
    f32 viewport_min_y;
    f32 viewport_max_x;
    f32 viewport_max_y;

    // Buffer allocated for the window.
    u8 *buffer;
}
Window;

#define PUN_KEYS_MAX 256

typedef struct Shader Shader;

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

    // Rendering window properties.
    // Set width, height and scale to this in init().
    //
    Window window;

    // Set to 0 to exit.
    //
    b32 running;
    Recorder recorder;

    u32 key_modifiers;
    
    // Indexed with KEY_* constants.
    // 0 for up
    // 1 for down
    //
    u8 key_states[PUN_KEYS_MAX];

    // Indexed with KEY_* constants.
    // 0 for not changed in this frame
    // 1 for changed in this frame
    //
    u8 key_deltas[PUN_KEYS_MAX];

    // Current mouse position.
    f32 mouse_x;
    f32 mouse_y;

    // String holding text that has been entered for the current frame.
#if PUNITY_TEXT_UNICODE
    u32 key_text[8];
#else
    char key_text[8];
#endif
    u8 key_text_length;

    // Total frame time.
    PerfSpan perf_frame;
    // Total time taken to step.
    PerfSpan perf_step;

    // Current frame number.
    i64 frame;
    // Current time.
    f32 time;
    f32 time_delta;

    // Color used to draw the window background.
    Color background;

    Canvas canvas;
    Palette *palette;
    DrawList *draw_list;

    f32 audio_volume;

    // Data for shader.
#ifdef PUN_SHADER_TYPE
    PUN_SHADER_TYPE *shader;
#endif

#if PUNITY_FEATURE_FILEWATCHER
    FileWatcher file_watcher;
#endif

    const char *args;
}
Core;

extern Core *CORE;

void panic_(const char *message, const char *description, const char *function, const char *file, int line);
void log_(const char *function, const char *file, int line, const char *format, ...);

#define LOG(message, ...) (log_(__FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__))
#define FAIL(message, ...) (LOG(message, __VA_ARGS__), assert(0))

// Asserts are to be used for unexpected program states.
#define ASSERT_MESSAGE(expression, message, ...) \
    (void)( (!!(expression)) ||\
            (panic_(message, #expression, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__), 0) )

#define ASSERT(expression) ASSERT_MESSAGE(expression, #expression)
#ifdef PUNITY_DEBUG
#define ASSERT_DEBUG_MESSAGE ASSERT_MESSAGE
#define ASSERT_DEBUG ASSERT
#else
#define ASSERT_DEBUG_MESSAGE
#define ASSERT_DEBUG
#endif

// To be defined in main.c.
extern int init();
// To be defined in main.c.
extern void step();

//
// Keys
//

// Returns true if key is held down.
//
#define key_down(key) (CORE->key_states[key])

// Returns true if key started to be held down in this frame.
//
#define key_pressed(key) (CORE->key_deltas[key] && key_down(key))

// Returns true if key stopped to be held down in this frame.
//
#define key_released(key) (CORE->key_deltas[key] && !key_down(key))

void key_clear();

enum
{
    KEY_MOD_SHIFT           = 0x01,
    KEY_MOD_CONTROL         = 0x02,
    KEY_MOD_ALT             = 0x03,
    KEY_MOD_SUPER           = 0x04
};

// Names follow Love2D's table where possible.
// https://love2d.org/wiki/KeyConstant
//
#define PUN_KEY_MAPPING \
    PUN_KEY_MAPPING_ENTRY(UNKNOWN             , "none",         0x00) \
    PUN_KEY_MAPPING_ENTRY(MOUSE_LEFT          , "mouseleft",    0x01) \
    PUN_KEY_MAPPING_ENTRY(MOUSE_RIGHT         , "mouseright",   0x02) \
    PUN_KEY_MAPPING_ENTRY(MOUSE_MIDDLE        , "mousemiddle",  0x04) \
    PUN_KEY_MAPPING_ENTRY(BACKSPACE           , "backspace",    0x08) \
    PUN_KEY_MAPPING_ENTRY(TAB                 , "tab",          0x09) \
    PUN_KEY_MAPPING_ENTRY(RETURN              , "return",       0x0D) \
    PUN_KEY_MAPPING_ENTRY(ESCAPE              , "escape",       0x1B) \
    PUN_KEY_MAPPING_ENTRY(PAGEUP              , "pageup",       0x21) \
    PUN_KEY_MAPPING_ENTRY(PAGEDOWN            , "pagedown",     0x22) \
    PUN_KEY_MAPPING_ENTRY(END                 , "end",          0x23) \
    PUN_KEY_MAPPING_ENTRY(HOME                , "home",         0x24) \
    PUN_KEY_MAPPING_ENTRY(LEFT                , "left",         0x25) \
    PUN_KEY_MAPPING_ENTRY(UP                  , "up",           0x26) \
    PUN_KEY_MAPPING_ENTRY(RIGHT               , "right",        0x27) \
    PUN_KEY_MAPPING_ENTRY(DOWN                , "down",         0x28) \
    PUN_KEY_MAPPING_ENTRY(PRINTSCREEN         , "printscreen",  0x2C) \
    PUN_KEY_MAPPING_ENTRY(F1                  , "f1",           0x70) \
    PUN_KEY_MAPPING_ENTRY(F2                  , "f2",           0x71) \
    PUN_KEY_MAPPING_ENTRY(F3                  , "f3",           0x72) \
    PUN_KEY_MAPPING_ENTRY(F4                  , "f4",           0x73) \
    PUN_KEY_MAPPING_ENTRY(F5                  , "f5",           0x74) \
    PUN_KEY_MAPPING_ENTRY(F6                  , "f6",           0x75) \
    PUN_KEY_MAPPING_ENTRY(F7                  , "f7",           0x76) \
    PUN_KEY_MAPPING_ENTRY(F8                  , "f8",           0x77) \
    PUN_KEY_MAPPING_ENTRY(F9                  , "f9",           0x78) \
    PUN_KEY_MAPPING_ENTRY(F10                 , "f10",          0x79) \
    PUN_KEY_MAPPING_ENTRY(F11                 , "f11",          0x7A) \
    PUN_KEY_MAPPING_ENTRY(F12                 , "f12",          0x7B) \
    PUN_KEY_MAPPING_ENTRY(SPACE               , "space",        ' ') \
    PUN_KEY_MAPPING_ENTRY(APOSTROPHE          , "\'",           '\'') \
    PUN_KEY_MAPPING_ENTRY(COMMA               , ",",            ',') \
    PUN_KEY_MAPPING_ENTRY(MINUS               , "-",            '-') \
    PUN_KEY_MAPPING_ENTRY(PERIOD              , ".",            '.') \
    PUN_KEY_MAPPING_ENTRY(SLASH               , "/",            '/') \
    PUN_KEY_MAPPING_ENTRY(0                   , "0",            '0') \
    PUN_KEY_MAPPING_ENTRY(1                   , "1",            '1') \
    PUN_KEY_MAPPING_ENTRY(2                   , "2",            '2') \
    PUN_KEY_MAPPING_ENTRY(3                   , "3",            '3') \
    PUN_KEY_MAPPING_ENTRY(4                   , "4",            '4') \
    PUN_KEY_MAPPING_ENTRY(5                   , "5",            '5') \
    PUN_KEY_MAPPING_ENTRY(6                   , "6",            '6') \
    PUN_KEY_MAPPING_ENTRY(7                   , "7",            '7') \
    PUN_KEY_MAPPING_ENTRY(8                   , "8",            '8') \
    PUN_KEY_MAPPING_ENTRY(9                   , "9",            '9') \
    PUN_KEY_MAPPING_ENTRY(SEMICOLON           , ";",            ';') \
    PUN_KEY_MAPPING_ENTRY(EQUAL               , "=",            '=') \
    PUN_KEY_MAPPING_ENTRY(A                   , "a",            'A') \
    PUN_KEY_MAPPING_ENTRY(B                   , "b",            'B') \
    PUN_KEY_MAPPING_ENTRY(C                   , "c",            'C') \
    PUN_KEY_MAPPING_ENTRY(D                   , "d",            'D') \
    PUN_KEY_MAPPING_ENTRY(E                   , "e",            'E') \
    PUN_KEY_MAPPING_ENTRY(F                   , "f",            'F') \
    PUN_KEY_MAPPING_ENTRY(G                   , "g",            'G') \
    PUN_KEY_MAPPING_ENTRY(H                   , "h",            'H') \
    PUN_KEY_MAPPING_ENTRY(I                   , "i",            'I') \
    PUN_KEY_MAPPING_ENTRY(J                   , "j",            'J') \
    PUN_KEY_MAPPING_ENTRY(K                   , "k",            'K') \
    PUN_KEY_MAPPING_ENTRY(L                   , "l",            'L') \
    PUN_KEY_MAPPING_ENTRY(M                   , "m",            'M') \
    PUN_KEY_MAPPING_ENTRY(N                   , "n",            'N') \
    PUN_KEY_MAPPING_ENTRY(O                   , "o",            'O') \
    PUN_KEY_MAPPING_ENTRY(P                   , "p",            'P') \
    PUN_KEY_MAPPING_ENTRY(Q                   , "q",            'Q') \
    PUN_KEY_MAPPING_ENTRY(R                   , "r",            'R') \
    PUN_KEY_MAPPING_ENTRY(S                   , "s",            'S') \
    PUN_KEY_MAPPING_ENTRY(T                   , "t",            'T') \
    PUN_KEY_MAPPING_ENTRY(U                   , "u",            'U') \
    PUN_KEY_MAPPING_ENTRY(V                   , "v",            'V') \
    PUN_KEY_MAPPING_ENTRY(W                   , "w",            'W') \
    PUN_KEY_MAPPING_ENTRY(X                   , "x",            'X') \
    PUN_KEY_MAPPING_ENTRY(Y                   , "y",            'Y') \
    PUN_KEY_MAPPING_ENTRY(Z                   , "z",            'Z') \
    PUN_KEY_MAPPING_ENTRY(LEFT_BRACKET        , "[",            '[') \
    PUN_KEY_MAPPING_ENTRY(BACK_SLASH          , "\\",            '\\') \
    PUN_KEY_MAPPING_ENTRY(RIGHT_BRACKET       , "]",            ']') \
    PUN_KEY_MAPPING_ENTRY(BACK_QUOTE          , "`",            '`') \

#define PUN_KEY_MAPPING_ENTRY(k, s, v) KEY_##k = v,
enum {
    PUN_KEY_MAPPING
};
#undef PUN_KEY_MAPPING_ENTRY

typedef struct KeyMapping {
    u8 key;
    u8 value;
    const char *name;
} KeyMapping;

extern KeyMapping KEY_MAPPING[];

// TODO:
// #if PUN_USE_SHORT_NAMES
// #define entity_add pun_entity_add
// #endif

#endif // PUNITY_H

//
//
//
// Implementation
//
//
//

#ifdef PUNITY_IMPLEMENTATION
#undef PUNITY_IMPLEMENTATION

#if PUN_PLATFORM_OSX || PUN_PLATFORM_LINUX
// TODO
#else
#define _WINSOCKAPI_
#define _WIN32_WINNT 0x0501
#define NOMINMAX
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dsound.h>
#endif

#if PUNITY_USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#if PUNITY_USE_STB_VORBIS && !PUN_TARGET_LUA
#include <stb_vorbis.c>
#undef R
#undef L
#undef C
#endif

#if PUNITY_FEATURE_RECORDER
#define GIFW_IMPLEMENTATION
#include <gifw.h>
#endif

#define PUNP_SOUND_DEFAULT_SOUND_VOLUME 0.9f
#define PUNP_SOUND_DEFAULT_MASTER_VOLUME 0.9f

// Set to 1 to output a audio.buf file from the mixer.
#define PUNP_SOUND_CHANNELS 2
#define PUNP_SOUND_BUFFER_CHUNK_COUNT 16
#define PUNP_SOUND_BUFFER_CHUNK_SAMPLES  3000
#define PUNP_SOUND_SAMPLES_TO_BYTES(samples, channels) ((samples) * (sizeof(i16) * channels))
#define PUNP_SOUND_BYTES_TO_SAMPLES(bytes, channels) ((bytes) / (sizeof(i16) * channels))
#define PUNP_SOUND_BUFFER_BYTES (PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_COUNT * PUNP_SOUND_BUFFER_CHUNK_SAMPLES, PUNP_SOUND_CHANNELS))

#define PUN_KEY_MAPPING_ENTRY(k, s, v) { KEY_##k, v, s },
KeyMapping KEY_MAPPING[] = {
    PUN_KEY_MAPPING
};
#undef PUN_KEY_MAPPING_ENTRY

#if PUNITY_SIMD

struct
{
    __m128i mm_00;
    __m128i mm_FF;
    __m128i mm_flip;
    __m128i masks[16];
}
simd__ = {0};

void
simd_init__()
{
    simd__.mm_00 = _mm_set1_epi8(0x00);
    simd__.mm_FF = _mm_set1_epi8(0xFF);
    simd__.masks[ 0] = simd__.mm_00;
    simd__.masks[ 1] = _mm_srli_si128(simd__.mm_FF, 15);
    simd__.masks[ 2] = _mm_srli_si128(simd__.mm_FF, 14);
    simd__.masks[ 3] = _mm_srli_si128(simd__.mm_FF, 13);
    simd__.masks[ 4] = _mm_srli_si128(simd__.mm_FF, 12);
    simd__.masks[ 5] = _mm_srli_si128(simd__.mm_FF, 11);
    simd__.masks[ 6] = _mm_srli_si128(simd__.mm_FF, 10);
    simd__.masks[ 7] = _mm_srli_si128(simd__.mm_FF,  9);
    simd__.masks[ 8] = _mm_srli_si128(simd__.mm_FF,  8);
    simd__.masks[ 9] = _mm_srli_si128(simd__.mm_FF,  7);
    simd__.masks[10] = _mm_srli_si128(simd__.mm_FF,  6);
    simd__.masks[11] = _mm_srli_si128(simd__.mm_FF,  5);
    simd__.masks[12] = _mm_srli_si128(simd__.mm_FF,  4);
    simd__.masks[13] = _mm_srli_si128(simd__.mm_FF,  3);
    simd__.masks[14] = _mm_srli_si128(simd__.mm_FF,  2);
    simd__.masks[15] = _mm_srli_si128(simd__.mm_FF,  1);

    simd__.mm_flip = _mm_set_epi64x(0x0001020304050607, 0x08090a0b0c0d0e0f);
}
#endif

Core *CORE = 0;

//
//
//

void
panic_(const char *message, const char *description, const char *function, const char *file, int line)
{
    LOG("ASSERTION FAILED:\n"
                   "\t    message: %s\n"
                   "\tdescription: %s\n"
                   "\t   function: %s\n"
                   "\t       file: %s\n"
                   "\t       line: %d\n",

           message,
           description,
           function,
           file,
           line);

    assert(0);
    // *(int *)0 = 0;
}

void
log_(const char *function, const char *file, int line, const char *format, ...)
{
#if PUNITY_DEBUG_MSVC
    char m[256];
    
    va_list args;
    va_start(args, format);
    StringCchVPrintfA(m, array_count(m), format, args);
    va_end(args);

    OutputDebugStringA(m);
#else
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}


//
// Utility
//

// may want to change your 1's to 1U for 32-bit and 1UL for 64bit

i32
i32_sign(i32 i) {
    return (i >> 31) | ((u32)-i >> 31);
}

i32
i32_abs(i32 i) {
    return (i + (i >> 31)) ^ (i >> 31);
    // return i & ~((u32)1 << 31);
}

i32
i32_positive(i32 i) {
    return !((u32)i >> 31);
}

i32
i32_negative(i32 i) {
    return ((u32)i >> 31);
}

//
// Random
//

#define RAND32_MAX 0xffffffff

u32
rand_u(u32 *x)
{
    if (*x == 0) *x = 314159265;
    *x ^= *x << 13;
    *x ^= *x >> 17;
    *x ^= *x << 5;
    return *x;
}

f32
rand_f(u32 *x)
{
    return (f32)(rand_u(x) % 1000) / 1000.0f;
}

f32
rand_fr(u32 *x, f32 min, f32 max)
{
    f32 f = rand_f(x);
    return min + (f * (max-min));
}

i32
rand_ir(u32 *x, i32 min, i32 max)
{
    i32 f = abs((i32)rand_u(x));
    return min + (f % ((max-min) + 1));
}

//
// Timer.
//

f64
perf_get()
{
#if PUN_PLATFORM_WINDOWS
    static i64 frequency_i = 0;
    static f64 frequency;
    if (frequency_i == 0) {
        QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_i);
        frequency = (f64)frequency_i;
    }
    i64 counter;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (f64)((f64)counter / frequency); // *(1e3);
#endif
}

void
perf_from(PerfSpan *span)
{
    span->stamp = perf_get();
}

void
perf_to(PerfSpan *span)
{
    f64 now = perf_get();
    span->delta = (f32)(now - span->stamp);
    span->stamp = now;
}

f32
perf_delta(const PerfSpan *span)
{
    f64 now = perf_get();
    return maximum(0.0f, (f32)(now - span->stamp));
}

//
// Strings
//

char *
string_push_0(char *dest, const char *src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
    return dest;
}

char *
string_push(char *dest, const char *src_it, const char *src_end)
{
    size_t length = src_end - src_it;
    memcpy(dest, src_it, length);
    return dest + length;
}

size_t
string_concat(char *buffer, size_t size, const char *str, ...)
{
    va_list args;
    const char *s;

    size_t length = strlen(str);
    va_start(args, str);
    while ((s = va_arg(args, char*))) {
        length += strlen(s);
    }
    va_end(args);

    if (length >= size) {
        ASSERT_MESSAGE(buffer == 0, "Insufficient buffer size.");
        return length + 1;
    }

//    char *res = malloc(len + 1);
//    if (!res) return NULL;

    strcpy(buffer, str);
    va_start(args, str);
    while ((s = va_arg(args, char*))) {
        strcat(buffer, s);
    }
    va_end(args);

    return length;
}

char *
string_copy_0(char *dest, size_t size, const char *str)
{
    ASSERT(size != 0);
    while (size && *str) {
        *dest++ = *str++;
        size--;
    }
    *dest = 0;
    return dest;
}


//
// File I/O
//

void *
file_read(Bank *bank, const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0L, SEEK_END);
        *size = ftell(f);
        fseek(f, 0L, SEEK_SET);

        u8 *ptr;
        if (bank == 0) {
            ptr = malloc(*size + 1);
        } else {
            ptr = bank_push(bank, *size + 1);
        }
        size_t read = fread(ptr, 1, *size, f);
        fclose(f);
        ASSERT(read == *size);
        ptr[*size] = 0;

        return ptr;
    }
    return 0;
}

bool
file_write(const char *path, void *ptr, size_t size)
{
    FILE *f = fopen(path, "wb+");
    if (f) {
        fwrite(ptr, size, 1, f);
        fclose(f);
        return true;
    }
    return false;
}

//
// Path
//

#define PATH_NORMALIZE(Ch) \
    switch (Ch) \
    { \
    case '\\': \
        Ch = '/'; \
        break; \
    case 0: \
        goto end; \
    }

char *
path_normalize_0(char *path)
{
    for (;;)
    {
        PATH_NORMALIZE(*path);
        path++;
    }
end:
    return path;
}

char *
path_normalize(char *it, char *end)
{
    while (it != end)
    {
        PATH_NORMALIZE(*it);
        it++;
    }
end:
    return it;
}

static const char *
path_trim_slashes_front_(const char *needle)
{
    if (path_is_slash(*needle)) {
        do { needle++; } while (path_is_slash(*needle));
    }
    return needle;
}

static const char *
path_trim_slashes_back_(const char *needle)
{
    const char *needle_end = needle + strlen(needle);
    while (needle_end != needle && path_is_slash(*(needle_end-1))) {
        needle_end--;
    }

    return needle_end;
}

static const char *
path_trim_slashes_(const char *needle, const char **needle_end)
{
    needle = path_trim_slashes_front_(needle);
    *needle_end = path_trim_slashes_back_(needle);

    return needle;
}

uint32_t
path_concat(char *buffer, uint32_t size, const char *arg1, ...)
{
    va_list args;
    const char *arg;
    const char *arg_it;
    const char *arg_end;

    arg_end = path_trim_slashes_back_(arg1);
    size_t length = arg_end - arg1;

    va_start(args, arg1);
    while ((arg = va_arg(args, char*)))
    {
        arg_it = path_trim_slashes_(arg, &arg_end);
        length += (arg_end - arg_it) ? (arg_end - arg_it) + 1 : 0;
    }
    va_end(args);

    if ((length + 1) > size) {
        ASSERT_MESSAGE(buffer == 0, "Insufficient buffer size.");
        return length + 1;
    }

    buffer[0] = 0;
    if (length)
    {
        char *it = buffer;

        arg_end = path_trim_slashes_back_(arg1);
        it = string_push(it, arg1, arg_end);

        va_start(args, arg1);
        while ((arg = va_arg(args, char*))) {
            arg_it = path_trim_slashes_(arg, &arg_end);
            if (arg_end - arg_it) {
                if (it != buffer) {
                    *it++ = PUN_PATH_SEPARATOR;
                }
                it = string_push(it, arg_it, arg_end);
            }
        }
        va_end(args);

        // ASSERT(it == (buffer + length));
        path_normalize(buffer, it);

        *it++ = 0;
    }

    return size;
}

const char *
path_extension_0(const char *path)
{
    const char *ret = strrchr(path, '.');
    return ret ? ret : (path + strlen(path));
}

const char *
path_file_name_0(const char *path)
{
    ASSERT(path);
    if (*path) {
        char *it = (char*)path + strlen(path);
        while (it != path) {
            it--;
            if (path_is_slash(*it)) {
                return it + 1;
            }
        }
        return it;
    }
    return 0;
}

const char *
path_file_name(const char *it, const char *end)
{
    ASSERT(it <= end);
    while (end != it)
    {
        end--;
        if (path_is_slash(*end)) {
            return end + 1;
        }
    }
    return end;
}

//
// Memory
//

void *
virtual_reserve(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
    ptr = VirtualAlloc(ptr, size, MEM_RESERVE, PAGE_NOACCESS);
    ASSERT(ptr);
#else
    ptr = mmap((void*)ptr, size,
               PROT_NONE,
               MAP_PRIVATE | MAP_ANON,
               -1, 0);
    ASSERT(ptr != MAP_FAILED);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
    return ptr;
}

void *
virtual_commit(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
    ptr = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    ASSERT(ptr);
#else
    // TODO: MAP_PRIVATE or MAP_SHARED?
    ptr = mmap(ptr, size,
               PROT_READ | PROT_WRITE,
               MAP_FIXED | MAP_SHARED | MAP_ANON,
               -1, 0);
    ASSERT(ptr != MAP_FAILED);

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
    return ptr;
}

void
virtual_decommit(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
    VirtualFree(ptr, size, MEM_DECOMMIT);
#else
    // instead of unmapping the address, we're just gonna trick
    // the TLB to mark this as a new mapped area which, due to
    // demand paging, will not be committed until used.

    mmap(ptr, size,
         PROT_NONE,
         MAP_FIXED | MAP_PRIVATE | MAP_ANON,
         -1, 0);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
}

void
virtual_free(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
    unused(size);
    VirtualFree((void *)ptr, 0, MEM_RELEASE);
#else
    msync(ptr, size, MS_SYNC);
    munmap(ptr, size);
#endif
}

void *
virtual_alloc(void *ptr, u32 size)
{
    return virtual_commit(virtual_reserve(ptr, size), size);
}

//
//
//

void
bank_init(Bank *bank, u32 capacity)
{
    ASSERT(bank);
    bank->begin = bank->it = bank->end = virtual_reserve(0, capacity);
    ASSERT(bank->begin);
    bank->cap = bank->begin + capacity;
}

void
bank_free(Bank *bank)
{
    if (bank->begin) {
        virtual_free(bank->begin, bank->cap - bank->begin);
        memset(bank, 0, sizeof(bank));
    }
}

void
bank_clear(Bank *bank)
{
    bank->it = bank->begin;
}

void
bank_zero(Bank *bank)
{
    memset(bank->begin, 0, bank->end - bank->begin);
}

void *
bank_push(Bank *stack, u32 size)
{
    ASSERT(stack);
    ASSERT(stack->begin);
    void *ptr;

    if ((stack->end - stack->it) < size) {
        if ((stack->cap - stack->it) < size) {
            printf("Not enought memory in bank (%d required, %d available).\n", (int)(stack->end - stack->it), size);
            ASSERT(0);
            return 0;
        } else {
            u32 additional_size = size - (stack->end - stack->it);
            additional_size = minimum((stack->cap - stack->it), align_to(additional_size, 4096));
            ptr = virtual_commit(stack->end, additional_size);
            ASSERT(ptr && ptr == stack->end);
            stack->end += additional_size;
        }
    }
    ptr = stack->it;
    stack->it += size;
    return ptr;
}

void
bank_pop(Bank *stack, void *ptr)
{
    ASSERT(stack);
    ASSERT(stack->begin);
    ASSERT((u8 *)ptr >= stack->begin && (u8 *)ptr <= stack->it);
    stack->it = ptr;
}

BankState
bank_begin(Bank *bank)
{
    BankState state;
    state.bank = bank;
    state.state = *bank;
    return state;
}

void
bank_end(BankState *state)
{
    *state->bank = state->state;
}

//
// Deque
//

void
deque_init(Deque *deque, size_t block_size)
{
    deque->pool = deque->first = deque->last = 0;
    deque->count = 0;
    deque->block_size = block_size;
    deque->size = 0;
}

void
deque_clear(Deque *deque)
{
    if (deque->first)
    {
        DequeBlock *block = deque->first;
        while (block)
        {
            block->it = block->begin;
            block = block->next;
        }

        deque->last->next = deque->pool;
        deque->pool = deque->first;
        deque->last = 0;
        deque->first = 0;
        deque->size = 0;
        deque->count = 0;
    }
}

void
deque_free(Deque *deque)
{
    deque_clear(deque);
    DequeBlock *next;
    DequeBlock *block = deque->pool;
    while (block)
    {
        next = block->next;
        virtual_free(block, sizeof(DequeBlock) + deque->block_size);
        block = next;
    }

    deque->first = 0;
    deque->last = 0;
    deque->pool = 0;
    deque->count = 0;
    deque->size = 0;
    // Keep block_size.
}

DequeBlock *
deque_block_alloc_(Deque *deque)
{
    DequeBlock *block = deque->pool;
    if (block) {
        deque->pool = block->next;
    } else {
        block = (DequeBlock*)virtual_alloc(0, sizeof(DequeBlock) + deque->block_size);
        block->begin = block->it = (uint8_t*)(block + 1);
        block->end = block->begin + deque->block_size;
        deque->count++;
    }
    block->next = 0;

    if (!deque->first) {
        deque->first = block;
    } else {
        deque->last->next = block;
    }
    deque->last = block;

    return block;
}

void *
deque_block_push(Deque *deque)
{
    DequeBlock *block = deque_block_alloc_(deque);
    deque->size += deque->block_size;
    block->it = block->end;
    return block->begin;
}

void *
deque_append(Deque *deque, void *data, size_t size)
{
    DequeBlock *block = deque->last;
    if (!block) {
        block = deque_block_alloc_(deque);
    }

    void *ret = block->it;

    size_t remaining;

    deque->size += size;
    while (size)
    {
        remaining = block->end - block->it;
        remaining = remaining > size ? size : remaining;
        memmove(block->it, data, remaining);
        block->it += remaining;
        size -= remaining;
        data = ((uint8_t*)data) + remaining;
        if (size == 0) {
            break;
        }
        block = deque_block_alloc_(deque);
    }

    return ret;
}

void *
deque_push(Deque *deque, size_t size)
{
    ASSERT_MESSAGE(size <= deque->block_size, "size is greater than the block size");
    DequeBlock *block = deque->last;
    if (!block || (block->end - block->it) < size) {
        block = deque_block_alloc_(deque);
    }

    void *ret = block->it;
    block->it += size;
    deque->size += size;
    return ret;
}

void
deque_walk(Deque *deque, DequeWalkF f, void *user)
{
    size_t total = deque->size;
    size_t passed = 0;
    DequeBlock *block = deque->first;
    while (block)
    {
        passed += block->it - block->begin;
        if (!f(deque, block->begin, block->it, (passed * 100)/total, user)) {
            break;
        }
        block = block->next;
    }
}

char *
deque_push_string(Deque *deque, const char *string) {
    return (char*)deque_append(deque, (char*)string, strlen(string));
}

DEQUE_PUSH_SCALAR_IMPLEMENT(u8, uint8_t);
DEQUE_PUSH_SCALAR_IMPLEMENT(i8, int8_t);
DEQUE_PUSH_SCALAR_IMPLEMENT(i16, int16_t);
DEQUE_PUSH_SCALAR_IMPLEMENT(u16, uint16_t);
DEQUE_PUSH_SCALAR_IMPLEMENT(i32, int32_t);
DEQUE_PUSH_SCALAR_IMPLEMENT(u32, uint32_t);

//
// Color
//

Color
color_make(u8 r, u8 g, u8 b, u8 a)
{
    Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

Color
color_lerp(Color from, Color to, f32 t)
{
    from.r = (i32)(lerp((f32)from.r, (f32)to.r, t));
    from.g = (i32)(lerp((f32)from.g, (f32)to.g, t));
    from.b = (i32)(lerp((f32)from.b, (f32)to.b, t));
    return from;
}

//
// Palette
//

void
palette_init(Palette *palette)
{
    memset(palette, 0, sizeof(Palette));
    
    // Skip the 0th color, as that's reserved for transparency.
    palette->ranges[0].begin = 1;
    palette->ranges[0].end   = 0xFF;
    palette->ranges[0].it    = 1;
    palette->ranges_count = 1;
}

void
palette_load(Palette *palette, const char *path, int offset)
{
    int width, height, comp;
    Color *pixels = (Color *)stbi_load(path, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    int count = width * height;
    Color c;
    Color *it = pixels;
    for (int i = 0; i != count; ++i, ++it, ++offset) {
        c.b = it->r;
        c.g = it->g;
        c.r = it->b;
        c.a = 0xFF;
        palette_color_set(palette, offset, c);
    }

    free(pixels);
}

void
palette_load_resource(Palette *palette, const char *resource_name, int offset)
{
    size_t size;
    void *ptr = resource_get(resource_name, &size);

    int width, height, comp;
    Color *pixels = (Color *)stbi_load_from_memory(ptr, (int)size, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    int count = width * height;
    Color c;
    Color *it = pixels;
    for (int i = 0; i != count; ++i, ++it, ++offset) {
        c.b = it->r;
        c.g = it->g;
        c.r = it->b;
        c.a = 0xFF;
        palette_color_set(palette, offset, c);
    }

    free(pixels);
}

int
palette_range_add(Palette *palette, int begin, int end, int it)
{
    ASSERT(palette->ranges_count < array_count(palette->ranges));
    PaletteRange *range = &palette->ranges[palette->ranges_count];
    range->begin = begin;
    range->end = end;
    range->it = it;

    return palette->ranges_count++;
}

u8
palette_color_set(Palette *palette, u8 index, Color color)
{
    palette->colors[index] = color;
    return index;
}

int
palette_color_add(Palette *palette, Color color, int range)
{
    ASSERT(range < palette->ranges_count);
    PaletteRange *r = &palette->ranges[range];
    if (r->it == r->end) {
        return -1;
    }
    LOG("Added color (%d, %d, %d, %d) to range %d at index %d.\n",
        color.r, color.g, color.b, color.a, range,
        r->it);
    palette->colors[r->it] = color;
    return r->it++;
}

int
palette_color_find(Palette *palette, Color color, int range)
{
    ASSERT(range < palette->ranges_count);
    PaletteRange *r = &palette->ranges[range];
    Color *it = &palette->colors[r->begin];
    for (int i = r->begin; i != r->it; ++i, ++it) {
        if (it->rgba == color.rgba) {
            return i;
        }
    }
    return -1;    
}

int
palette_color_find_fuzzy(Palette *palette, Color color, int range)
{
    ASSERT(range < palette->ranges_count);
    PaletteRange *r = &palette->ranges[range];
    int closest = -1;
    int closest_distance = INT_MAX;
    int distance = 0;
    Color *it = &palette->colors[r->begin];
    for (int i = r->begin; i != r->it; ++i, ++it) {
        distance =
            i32_abs((int)it->r - (int)color.r) +
            i32_abs((int)it->g - (int)color.g) +
            i32_abs((int)it->b - (int)color.b);

        if (distance < closest_distance) {
            closest = i;
            closest_distance = distance;
        }
    }

    return closest;
}

int
palette_color_acquire(Palette *palette, Color color, int range)
{
    int index = palette_color_find(palette, color, range);
    if (index == -1) {
        index = palette_color_add(palette, color, range);
    }
    return index;
}

//
// Rectangle
//

Rect
rect_make(i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    Rect r;
    r.min_x = min_x;
    r.min_y = min_y;
    r.max_x = max_x;
    r.max_y = max_y;
    return r;
}

Rect
rect_make_size(i32 x, i32 y, i32 w, i32 h)
{
    Rect r = rect_make(x,
                       y,
                       x + w,
                       y + h);
    return r;
}

Rect
rect_make_centered(i32 x, i32 y, i32 w, i32 h)
{
    w = w / 2;
    h = h / 2;
    Rect r = rect_make(x - w,
                       y - h,
                       x + w,
                       y + h);
    return r;
}

Rect
rect_clip(Rect rect, Rect clip)
{
    rect.min_x = maximum(clip.min_x,     minimum(clip.max_x, rect.min_x));
    rect.min_y = maximum(clip.min_y,     minimum(clip.max_y, rect.min_y));
    rect.max_x = maximum(rect.min_x,     minimum(clip.max_x, rect.max_x));
    rect.max_y = maximum(rect.min_y,     minimum(clip.max_y, rect.max_y));
    return rect;
}

bool
rect_contains_point(Rect rect, i32 x, i32 y)
{
    return (x >= rect.min_x && x < rect.max_x &&
            y >= rect.min_y && y < rect.max_y);
}

bool
rect_overlaps(Rect rect_a, Rect rect_b)
{
    return !(((rect_b.min_x - rect_a.max_x) > 0 || (rect_b.min_y - rect_a.max_y) > 0) ||
             ((rect_a.min_x - rect_b.max_x) > 0 || (rect_a.min_y - rect_b.max_y) > 0));
}

void
rect_center(Rect rect, i32 *x, i32 *y) {
    *x = (rect.min_x + rect.max_x) / 2;
    *y = (rect.min_y + rect.max_y) / 2;
}

bool
rect_check_limits(Rect *rect, i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    return (rect->min_x >= min_x) && (rect->min_y >= min_y) &&
           (rect->max_x <= max_x) && (rect->max_y <= max_y) &&
           (rect->min_x <= rect->max_x) &&
           (rect->min_y <= rect->max_y);
}

//
// DrawList
//

void
drawlist_init(DrawList *list, size_t reserve)
{
    list->items = 0;
    list->items_count = 0;
    list->items_reserve = reserve;
    list->items_additional = 0;
}

void
drawlist_begin(DrawList *list)
{
    list->items_reserve += align_to(list->items_additional, 256);
    list->items = (DrawListItem **)bank_push(CORE->stack, sizeof(DrawListItem*) * list->items_reserve);
    list->items_storage = (DrawListItem *)bank_push(CORE->stack, sizeof(DrawListItem) * list->items_reserve);
    list->items_count = 0;
    list->items_additional = 0;
}

static DrawListItem **
drawlist_sort_(DrawListItem **entries, u32 count, DrawListItem **temp)
{
    DrawListItem **dest = temp;
    DrawListItem **source = entries;

    for (u32 bi = 0; bi != 4*8; bi += 4)
    {
        u32 offsets[256] = {0};

        // Count.
        for (u32 i = 0; i != count; ++i)
        {
            u32 value = (u32)source[i]->key;
            u32 piece = (value >> bi) & 0xff;
            offsets[piece]++;
        }

        // Counts to offsets.
        u32 total = 0;
        for (u32 si = 0; si != array_count(offsets); ++si)
        {
            u32 count = offsets[si];
            offsets[si] = total;
            total += count;
        }

        // Place.
        for (u32 i = 0; i != count; ++i)
        {
            u32 value = (u32)source[i]->key;
            u32 piece = (value >> bi) & 0xff;
            dest[offsets[piece]++] = source[i];
        }

        DrawListItem **t = dest;
        dest = source;
        source = t;
    }

    return source;
}

void
drawlist_end(DrawList *list)
{
    if (list->items_count == 0) {
        return;
    }

    list->perf = perf_get();
    Canvas canvas = CORE->canvas;
    DrawListItem **temp = (DrawListItem **)bank_push(CORE->stack,
        sizeof(DrawListItem*) * list->items_count);
    DrawListItem **it = drawlist_sort_(list->items, list->items_count, temp);
    DrawListItem *item;
    for (size_t i = 0; i != list->items_count; ++i, ++it)
    {
        item = *it;
        CORE->canvas = item->canvas;
        switch (item->type)
        {
        case DrawListItemType_Line:
            line_draw(item->line.x1,
                item->line.y1,
                item->line.x2,
                item->line.y2,
                item->line.color);
            break;
        case DrawListItemType_Rect:
            rect_draw(item->rect.rect, item->rect.color);
            break;
        case DrawListItemType_Frame:
            ASSERT_MESSAGE(0, "Not implemented, yet.");
            // frame_draw(item->rect.rect, item->rect.color);
            break;
        case DrawListItemType_Text:
            text_draw(item->text.text, item->text.x, item->text.y, item->text.color);
            break;
        case DrawListItemType_BitmapFull:
        case DrawListItemType_BitmapPartial:
            bitmap_draw(
                item->bitmap.bitmap,
                item->bitmap.x,
                item->bitmap.y,
                item->bitmap.pivot_x,
                item->bitmap.pivot_y,
                item->type == DrawListItemType_BitmapPartial ? &item->bitmap.bitmap_rect : 0);
            break;

        case DrawListItemType_Callback:
            item->callback.callback(item->callback.data, item->callback.data_size);
            break;

        default:
            ASSERT_MESSAGE(0, "Unknown item type.");
            break;
        }
    }
    list->perf = (perf_get() - list->perf);
    bank_pop(CORE->stack, temp);
    CORE->canvas = canvas;
}

void
drawlist_clear(DrawList *list)
{
    list->items_count = 0;
}

//
// Pushed draw functions
//

static DrawListItem *
drawlist_push_(DrawList *list, i32 z, u32 type)
{
    if (list->items_count == list->items_reserve) {
        if (list->items_additional == 0) {
            printf("Not enough space for new items in drawlist_push.\n");
            printf("Allocating more space for next frame.\n");
        }
        list->items_additional++;
        return 0;
    }

    DrawListItem *item = &list->items_storage[list->items_count];
    item->type = type;
    // Convert from i32 to u32 by converting
    // from INT32_MIN -> INT32_MAX
    //   to 0 -> UINT32_MAX
    item->key = (u32)(z - INT32_MIN);
    item->canvas = CORE->canvas;

    list->items[list->items_count] = item;
    list->items_count++;

    return item;
}

DrawListItem *
drawlist_callback_push_blob(DrawList *list, DrawListCallbackF *callback, void *data, size_t data_size, i32 z)
{
    DrawListItem *item = drawlist_push_(list, z, DrawListItemType_Callback);
    if (item) {
        item->callback.callback = callback;
        item->callback.data = bank_push(CORE->stack, data_size);
        item->callback.data_size = data_size;
        memmove(item->callback.data, data, data_size);
    }
    return item;
}

DrawListItem *
drawlist_callback_push_ptr(DrawList *list, DrawListCallbackF *callback, void *ptr, i32 z)
{
    DrawListItem *item = drawlist_push_(list, z, DrawListItemType_Callback);
    if (item) {
        item->callback.callback = callback;
        item->callback.data = ptr;
        item->callback.data_size = 0;
    }
    return item;
}

DrawListItem *
line_draw_push(i32 x1, i32 y1, i32 x2, i32 y2, u8 color, i32 z)
{
    DrawListItem *item = drawlist_push_(CORE->draw_list, z, DrawListItemType_Line);
    if (item) {
        item->line.x1 = x1;
        item->line.y1 = y1;
        item->line.x2 = x2;
        item->line.y2 = y2;
        item->line.color = color;
    }
    return item;
}

DrawListItem *
rect_draw_push(Rect rect, u8 color, i32 z)
{
    DrawListItem *item = drawlist_push_(CORE->draw_list, z, DrawListItemType_Rect);
    if (item) {
        item->rect.rect = rect;
        item->rect.color = color;
    }
    return item;
}

DrawListItem *
frame_draw_push(Rect r, u8 frame_color, int frame_edges, u8 fill_color, i32 z)
{
    Rect fill = r;
    if (frame_color != PUN_COLOR_TRANSPARENT)
    {
        if (frame_edges & Edge_Left) {
            rect_draw_push(rect_make(r.min_x, r.min_y, r.min_x + 1, r.max_y), frame_color, z);
            fill.min_x++;
        }
        if (frame_edges & Edge_Right) {
            rect_draw_push(rect_make(r.max_x - 1, r.min_y, r.max_x, r.max_y), frame_color, z);
            fill.max_x--;
        }
        if (frame_edges & Edge_Top) {
            rect_draw_push(rect_make(r.min_x, r.min_y, r.max_x, r.min_y + 1), frame_color, z);
            fill.min_y++;
        }
        if (frame_edges & Edge_Bottom) {
            rect_draw_push(rect_make(r.min_x, r.max_y - 1, r.max_x, r.max_y), frame_color, z);
            fill.max_y--;
        }
    }

    if (fill_color != PUN_COLOR_TRANSPARENT && fill.min_x < fill.max_x && fill.min_y < fill.max_y) {
        return rect_draw_push(fill, fill_color, z);
    }
    return 0;
}

DrawListItem *
bitmap_draw_push(Bitmap *bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect, i32 z)
{
    DrawListItem *item = drawlist_push_(CORE->draw_list, z, DrawListItemType_BitmapFull);
    if (item) {
        item->bitmap.bitmap = bitmap;
        item->bitmap.x = x;
        item->bitmap.y = y;
        item->bitmap.pivot_x = pivot_x;
        item->bitmap.pivot_y = pivot_y;
        if (bitmap_rect) {
            ASSERT(rect_check_limits(bitmap_rect, 0, 0, bitmap->width, bitmap->height));
            item->bitmap.bitmap_rect = *bitmap_rect;
            item->type = DrawListItemType_BitmapPartial;
        }
    }
    return item;
}

DrawListItem *
tile_draw_push(Bitmap *bitmap, i32 x, i32 y, i32 index, i32 z)
{
    Rect rect = tile_get(bitmap, index);
    return bitmap_draw_push(bitmap, x, y, 0, 0, &rect, z);
}

DrawListItem *
text_draw_push(const char *text, i32 x, i32 y, u8 color, i32 z)
{
    DrawListItem *item = drawlist_push_(CORE->draw_list, z, DrawListItemType_Text);
    if (item) {
        size_t text_length = strlen(text) + 1;
        item->text.text = (char*)bank_push(CORE->stack, text_length);
        memcpy(item->text.text, text, text_length);
        item->text.x = x;
        item->text.y = y;
        item->text.color = color;
    }
    return item;
}

//
// Debug
//

void
debug_palette(i32 x, i32 y, i32 z)
{
    for (int y = 0; y != 16; ++y) {
        for (int x = 0; x != 16; ++x) {
            rect_draw_push(rect_make_size(x * 4, y * 4, 4, 4), x + y * 16, z);
        }
    }
}

//
// V2f
//

V2f
v2f_make(f32 x, f32 y)
{
    V2f v;
    v.x = x;
    v.y = y;
    return v;
}

f32
v2f_length(V2f v)
{
    return sqrtf((v.x * v.x) + (v.y * v.y));
}

V2f
v2f_normalize(V2f v)
{
    V2f ret;
    f32 length = v2f_length(v);
    if (length == 0.0f) {
        ret = v;
    } else {
        ret = v2f_make(v.x / length, v.y / length);
    }
    return ret;
}

f32
v2f_angle(V2f v)
{
    return atan2f(v.y, v.x);
}

V2f
v2f_minus(V2f a, V2f b)
{
    return v2f_make(a.x - b.x, a.y - b.y);
}

//
// Canvas
//

void
rect_intersect(Rect *R, Rect *C)
{
    R->min_x = minimum(C->max_x, maximum(R->min_x, C->min_x));
    R->min_y = minimum(C->max_y, maximum(R->min_y, C->min_y));
    R->max_x = maximum(C->min_x, minimum(R->max_x, C->max_x));
    R->max_y = maximum(C->min_y, minimum(R->max_y, C->max_y));
}

// Returns 0 if R does not intersect with C (is fully invisible).
// Returns 1 if R has NOT been clipped (is fully visible).
// Returns 2 if R has been clipped.
i32
clip_rect_with_offsets(Rect *R, Rect *C, i32 *ox, i32 *oy)
{
    if (R->max_x < C->min_x || R->min_x >= C->max_x ||
        R->max_y < C->min_y || R->min_y >= C->max_y) {
        return 0;
    }

    i32 res = 1;

    if (R->min_x < C->min_x) {
        *ox = C->min_x - R->min_x;
        R->min_x = C->min_x;
        res = 2;
    } else if (R->max_x > C->max_x) {
        R->max_x = C->max_x;
        res = 2;
    }

    if (R->min_y < C->min_y) {
        *oy = C->min_y - R->min_y;
        R->min_y = C->min_y;
        res = 2;
    } else if (R->max_y > C->max_y) {
        R->max_y = C->max_y;
        res = 2;
    }

    return res;
}

void
clip_set(Rect rect)
{
    Rect canvas_rect = rect_make_size(0, 0, CORE->canvas.bitmap->width, CORE->canvas.bitmap->height);
    rect_intersect(&rect, &canvas_rect);
    CORE->canvas.clip = rect;
}

void
clip_reset()
{
    CORE->canvas.clip = rect_make_size(0, 0, CORE->canvas.bitmap->width, CORE->canvas.bitmap->height);
}

bool
clip_check()
{
    return (CORE->canvas.clip.min_x >= 0) &&
           (CORE->canvas.clip.min_y >= 0) &&
           (CORE->canvas.clip.max_x <= CORE->canvas.bitmap->width) &&
           (CORE->canvas.clip.max_y <= CORE->canvas.bitmap->height) &&
           (CORE->canvas.clip.min_x <= CORE->canvas.clip.max_x) &&
           (CORE->canvas.clip.min_y <= CORE->canvas.clip.max_y);
}

//
// Resources
//

void *
resource_get(const char *name, size_t *size)
{
#if PUN_PLATFORM_WINDOWS
    HRSRC handle = FindResource(0, name, "RESOURCE");
    ASSERT(handle);

    HGLOBAL data = LoadResource(0, handle);
    ASSERT(data);

    void *ptr = LockResource(data);
    ASSERT(ptr);

    DWORD t_size = SizeofResource(0, handle);
    ASSERT(t_size);

    *size = t_size;
    return ptr;
#endif
}

//
// Canvas
//

void
camera_reset()
{
    CORE->canvas.translate_x = 0;
    CORE->canvas.translate_y = 0;
}

void
camera_set(i32 x, i32 y, Rect *clip)
{
    int center_x = CORE->window.width  / 2;
    int center_y = CORE->window.height / 2;
    if (clip) {
        x = clamp(x, (clip->min_x + center_x), (clip->max_x - center_x));
        y = clamp(y, (clip->min_y + center_y), (clip->max_y - center_y));
    }
    // TODO: Use canvas.clip
    CORE->canvas.translate_x = -x + center_x;
    CORE->canvas.translate_y = -y + center_y;
}

i32
camera_x() {
    return -(CORE->canvas.translate_x - (CORE->window.width / 2));
}

i32
camera_y() {
    return -(CORE->canvas.translate_y - (CORE->window.height / 2));
}

void
canvas_clear(u8 color)
{
    rect_draw(CORE->canvas.clip, color);
    // TODO: This should clear only within the clip, so
    //       essentially we're drawing a rectangle here.
    // memset(CORE->canvas.bitmap->pixels, color, CORE->canvas.bitmap->width * CORE->canvas.bitmap->height);
}

void
pixel_draw_(i32 x, i32 y, u8 color) {
    *(CORE->canvas.bitmap->pixels + x + (y * CORE->canvas.bitmap->width)) = color;
}

void
pixel_draw(i32 x, i32 y, u8 color)
{
    x += CORE->canvas.translate_x;
    y += CORE->canvas.translate_y;
    if (rect_contains_point(CORE->canvas.clip, x, y)) {
        pixel_draw_(x, y, color);
    }
}

void
line_draw(i32 x1, i32 y1, i32 x2, i32 y2, u8 color)
{
    i32 x, y;
    i32 dx, dy;
    i32 error;
    i32 ystep;
    i32 steep = abs(y2 - y1) > abs(x2 - x1);
    
    if (steep) {
        swap_t(i32, x1, y1);
        swap_t(i32, x2, y2);
    }
    
    if (x1 > x2) {
        swap_t(i32, x1, x2);
        swap_t(i32, y1, y2);
    }

    dx = x2 - x1;
    dy = abs(y2 - y1);
    error = dx / 2;
    ystep = (y1 < y2) ? 1 : -1;
    y = y1;
    for (x = x1; x <= x2; x++)
    {
        if (steep) {
            pixel_draw(y, x, color);
        } else {
            pixel_draw(x, y, color);
        }
        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

void
rect_draw(Rect r, u8 color)
{
    rect_tr(&r, CORE->canvas.translate_x, CORE->canvas.translate_y);
    rect_intersect(&r, &CORE->canvas.clip);
    if (r.max_x > r.min_x && r.max_y > r.min_y)
    {
        u32 canvas_pitch = CORE->canvas.bitmap->width;
        u8 *row = CORE->canvas.bitmap->pixels + r.min_x + (r.min_y * canvas_pitch);
        i32 w = r.max_x - r.min_x;
        while (r.max_y != r.min_y)
        {
            memset(row, color, w);
            row += canvas_pitch;
            r.max_y--;
        }
    }
}

void
frame_draw(Rect r, u8 frame_color, int frame_edges, u8 fill_color)
{
    Rect fill = r;
    if (frame_edges & Edge_Left) {
        rect_draw(rect_make(r.min_x, r.min_y, r.min_x + 1, r.max_y), frame_color);
        fill.min_x++;
    }
    if (frame_edges & Edge_Right) {
        rect_draw(rect_make(r.max_x - 1, r.min_y, r.max_x, r.max_y), frame_color);
        fill.max_x--;
    }
    if (frame_edges & Edge_Top) {
        rect_draw(rect_make(r.min_x, r.min_y, r.max_x, r.min_y + 1), frame_color);
        fill.min_y++;
    }
    if (frame_edges & Edge_Bottom) {
        rect_draw(rect_make(r.min_x, r.max_y - 1, r.max_x, r.max_y), frame_color);
        fill.max_y++;
    }
    if (fill_color != PUN_COLOR_TRANSPARENT && fill.min_x < fill.max_x && fill.min_y < fill.max_y) {
        rect_draw(fill, fill_color);
    }
}

#if PUNITY_SIMD
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=SSE,SSE2,SSE3
// TODO: Use _mm_loadu_si128((__m128i*)(dit));?

void
bitmap_draw_simd_hflip_(Bitmap *d_bmp, Bitmap *s_bmp, u8 *d, u8 *s, int spitch, int dpitch, int sw, int sh, int mask)
{
    int pad = sw & (15);
    dpitch -= sw - pad;
    spitch += sw - pad;
    s  -= 16;
    sw -= 16;

    __m128i mm_clip = simd__.masks[pad];
    __m128i mm;
    int sy, sx;

    if (!mask)
    {
        for (sy = 0; sy != sh; ++sy, d += dpitch, s += spitch)
        {
            for (sx = 0; sx <= sw; sx += 16, s -= 16, d += 16)
            {
                mm = _mm_shuffle_epi8(_mm_lddqu_si128((__m128i*)(s)), simd__.mm_flip);
                _mm_storeu_si128((__m128i*)(d), _mm_or_si128(_mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), _mm_cmpeq_epi8(mm, simd__.mm_00)), mm));
            }

            if (pad) {
                mm = _mm_and_si128(_mm_shuffle_epi8(_mm_lddqu_si128((__m128i*)(s)), simd__.mm_flip), mm_clip);
                _mm_storeu_si128((__m128i*)(d), _mm_or_si128(_mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), _mm_cmpeq_epi8(mm, simd__.mm_00)), mm));
            }
        }
    }
    else
    {
        __m128i mm_mask = _mm_set1_epi8(mask);

        for (sy = 0; sy != sh; ++sy, d += dpitch, s += spitch)
        {
            for (sx = 0; sx <= sw; sx += 16, s -= 16, d += 16)
            {
                mm = _mm_cmpeq_epi8(_mm_shuffle_epi8(_mm_lddqu_si128((__m128i*)(s)), simd__.mm_flip), simd__.mm_00);
                _mm_storeu_si128((__m128i*)(d),
                    _mm_or_si128(
                        _mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), mm),
                        _mm_andnot_si128(mm, mm_mask)
                    )
                );
            }

            if (pad) {
                mm = _mm_cmpeq_epi8(_mm_and_si128(_mm_shuffle_epi8(_mm_lddqu_si128((__m128i*)(s)), simd__.mm_flip), mm_clip), simd__.mm_00);
                _mm_storeu_si128((__m128i*)(d),
                    _mm_or_si128(
                        _mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), mm),
                        _mm_andnot_si128(mm, mm_mask)
                    )
                );
            }
        }
    }
}

void
bitmap_draw_simd_nflip_(Bitmap *d_bmp, Bitmap *s_bmp, u8 *d, u8 *s, int spitch, int dpitch, int sw, int sh, int mask)
{
    int pad = sw & 15;
    int mul = sw & (~15);
    dpitch -= mul;
    spitch -= mul;
    sw -= 16;

    __m128i mm_clip = simd__.masks[pad];
    __m128i mm;
    int sy, sx;

    if (!mask)
    {
        for (sy = 0; sy != sh; ++sy, d += dpitch, s += spitch)
        {
            for (sx = 0; sx <= sw; sx += 16, s += 16, d += 16)
            {
                mm = _mm_lddqu_si128((__m128i*)(s));
                _mm_storeu_si128((__m128i*)(d), _mm_or_si128(_mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), _mm_cmpeq_epi8(mm, simd__.mm_00)), mm));
            }

            if (pad) {
                mm = _mm_and_si128(_mm_lddqu_si128((__m128i*)(s)), mm_clip);
                _mm_storeu_si128((__m128i*)(d), _mm_or_si128(_mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), _mm_cmpeq_epi8(mm, simd__.mm_00)), mm));
            }
        }
    }
    else
    {
        __m128i mm_mask = _mm_set1_epi8(mask);

        for (sy = 0; sy != sh; ++sy, d += dpitch, s += spitch)
        {
            for (sx = 0; sx <= sw; sx += 16, s += 16, d += 16)
            {
                mm = _mm_cmpeq_epi8(_mm_lddqu_si128((__m128i*)(s)), simd__.mm_00);
                _mm_storeu_si128((__m128i*)(d),
                    _mm_or_si128(
                        _mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), mm),
                        _mm_andnot_si128(mm, mm_mask)
                    )
                );
            }

            if (pad) {
                mm = _mm_cmpeq_epi8(_mm_and_si128(_mm_lddqu_si128((__m128i*)(s)), mm_clip), simd__.mm_00);
                _mm_storeu_si128((__m128i*)(d),
                    _mm_or_si128(
                        _mm_and_si128(_mm_lddqu_si128((__m128i*)(d)), mm),
                        _mm_andnot_si128(mm, mm_mask)
                    )
                );
            }
        }
    }
}


void
bitmap_draw_simd_(Bitmap *s_bmp, int x, int y, int px, int py, Rect *clip)
{
    int mask = CORE->canvas.mask;
    int flag = CORE->canvas.flags;
    Bitmap *d_bmp = CORE->canvas.bitmap;

    // Source rectangle.
    Rect s_r = rect_make_size(0, 0, s_bmp->width, s_bmp->height);;
    if (clip) {
        rect_intersect(&s_r, clip);
    }

    // Destination rectangle.
    Rect d_r = rect_make_size(x - px, y - py,
                              s_r.max_x - s_r.min_x,
                              minimum(d_bmp->height, s_r.max_y - s_r.min_y));

    // Translate.
    rect_tr(&d_r, CORE->canvas.translate_x, CORE->canvas.translate_y);

    // Check if there's anything to draw.
    // If there is, get the offsets to the source rectangle.
    i32 s_ox = 0;
    i32 s_oy = 0;
    if (!clip_rect_with_offsets(&d_r, &CORE->canvas.clip, &s_ox, &s_oy)) {
        return;
    }

    int dw = rect_width(&d_r);
    int dh = rect_height(&d_r);
    int sw = rect_width(&s_r);
    int sh = rect_height(&s_r);

    int s_step_y = 0;
    int s_step_x = 0;
    int d_step_y = d_bmp->width;
    
    u8 *s = s_bmp->pixels;
    u8 *d = d_bmp->pixels + d_r.min_x + (d_r.min_y * d_bmp->width);

    if (flag & DrawFlags_FlipV)
    {
        // s_r.min_y += sh - dh - s_oy;
        s_r.max_y = s_r.min_y + dh;
        s += (s_r.max_y-1) * s_bmp->width;
        s_step_y -= s_bmp->width;
    }
    else
    {
        s_r.min_y += s_oy;
        // s_r.max_y = s_r.min_y + dh;
        s += s_r.min_y * s_bmp->width;
        s_step_y += s_bmp->width;
    }
    
    if (flag & DrawFlags_FlipH)
    {
        // s_r.min_x += sw - dw - s_ox;
        s_r.max_x = s_r.min_x + dw;
        s += s_r.max_x;
        bitmap_draw_simd_hflip_(d_bmp, s_bmp, d, s, s_step_y, d_step_y, dw, dh, mask);
    }
    else
    {
        s_r.min_x += s_ox;
        // s_r.max_x = s_r.min_x + dw;
        s += s_r.min_x;
        bitmap_draw_simd_nflip_(d_bmp, s_bmp, d, s, s_step_y, d_step_y, dw, dh, mask);
    }
}
#endif

void
bitmap_draw_single_(Bitmap *src_bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect)
{
    u32 flags = CORE->canvas.flags;
    u8 mask = CORE->canvas.mask;

    ASSERT(src_bitmap);
    ASSERT(clip_check());

    Rect src_r;
    if (bitmap_rect) {
        ASSERT(rect_check_limits(bitmap_rect, 0, 0, src_bitmap->width, src_bitmap->height));
        src_r = *bitmap_rect;
    } else {
        src_r = rect_make_size(0, 0, src_bitmap->width, src_bitmap->height);
    }

    // I'm not very sure about this, maybe introduce a FlipPivotH flag?
    // if (flags & DrawFlags_FlipH) {
    //     pivot_x = (src_r.max_x - src_r.min_x) - pivot_x;
    // }

    Bitmap *dst_bitmap = CORE->canvas.bitmap;
    Rect dst_r = rect_make_size(x - pivot_x, y - pivot_y,
                                src_r.max_x - src_r.min_x,
                                minimum(dst_bitmap->height, src_r.max_y - src_r.min_y));
    rect_tr(&dst_r, CORE->canvas.translate_x, CORE->canvas.translate_y);
    i32 src_ox = 0;
    i32 src_oy = 0;
    if (clip_rect_with_offsets(&dst_r, &CORE->canvas.clip, &src_ox, &src_oy))
    {
        i32 dst_w = dst_r.max_x - dst_r.min_x;
        i32 dst_h = dst_r.max_y - dst_r.min_y;
        i32 src_w = src_r.max_x - src_r.min_x;
        i32 src_h = src_r.max_y - src_r.min_y;

        i32 dst_step_y = dst_bitmap->width - dst_w;
        i32 dst_step_x = +1;
        u8 *dst = dst_bitmap->pixels;
        dst += dst_r.min_x + (dst_r.min_y * dst_bitmap->width);

        i32 src_step_y, src_step_x;
        u8 *src = src_bitmap->pixels;

        if (flags & DrawFlags_FlipH) {
            src_r.min_x += src_w - dst_w - src_ox;
            src_r.max_x = src_r.min_x + dst_w;
            src += src_r.max_x - 1;
            src_step_y = +dst_w;
            src_step_x = -1;
        } else {
            src_r.min_x += src_ox;
            src_r.max_x = src_r.min_x + dst_w;
            src += src_r.min_x;
            src_step_y = -dst_w;
            src_step_x = +1;
        }

        if (flags & DrawFlags_FlipV) {
            src_r.min_y += src_h - dst_h - src_oy;
            src_r.max_y = src_r.min_y + dst_h;
            src += (src_r.max_y-1) * src_bitmap->width;
            src_step_y += -src_bitmap->width;
        } else {
            src_r.min_y += src_oy;
            src_r.max_y = src_r.min_y + dst_h;
            src += src_r.min_y * src_bitmap->width;
            src_step_y += src_bitmap->width;
        }

        i32 y_, x_;

        if (flags & DrawFlags_Mask)
        {
            for (y_ = 0; y_ != dst_h; ++y_) {
                for (x_ = 0; x_ != dst_w; ++x_) {
                    *dst = *src ? mask : *dst;
                    src += src_step_x;
                    dst += dst_step_x;
                }
                dst += dst_step_y;
                src += src_step_y;
            }
        }
        else
        {
            for (y_ = 0; y_ != dst_h; ++y_) {
                for (x_ = 0; x_ != dst_w; ++x_) {
                    *dst = *src ? *src : *dst;
                    src += src_step_x;
                    dst += dst_step_x;
                }
                dst += dst_step_y;
                src += src_step_y;
            }
        }
    }
}

Rect
tile_get(Bitmap *bitmap, i32 index)
{
    ASSERT_MESSAGE(bitmap->tile_width,  "Bitmap does not have a `tile_width` set.");
    ASSERT_MESSAGE(bitmap->tile_height, "Bitmap does not have a `tile_height` set.");

    i32 cols = bitmap->width / bitmap->tile_width;
    Rect rect = rect_make_size(
        (index % cols) * bitmap->tile_width,
        (index / cols) * bitmap->tile_height,
        bitmap->tile_width, bitmap->tile_height);

    return rect;
}

void
tile_draw(Bitmap *bitmap, i32 x, i32 y, i32 index)
{
    Rect rect = tile_get(bitmap, index);
    bitmap_draw(bitmap, x, y, 0, 0, &rect);
}

void
bitmap_copy(Bitmap *destination, Bitmap *source)
{
    ASSERT(destination->width >= source->width);
    ASSERT(destination->height >= source->height);

    if (destination->width == source->width &&
        destination->height == source->height) {
        memcpy(destination->pixels, source->pixels,
            source->width * source->height);
    } else {
        u8 *drow = destination->pixels;
        u8 *srow = source->pixels;
        for (int y = 0; y != source->height; ++y) {
            memcpy(drow, srow, source->width);
            drow += destination->width;
            srow += source->width;
        }
    }
}

V2
text_measure(const char *text)
{
    V2 size = {0, 1};
    Font *font = CORE->canvas.font;
    while (*text)
    {
        switch (*text) {
        case '\n':
            size.y += 1;
            break;
        default:
            size.x += 1;
            break;
        }
        text++;
    }
    size.x *= font->char_width;
    size.y *= font->char_height;
    return size;
}

void
text_draw(const char *text, i32 x, i32 y, u8 color)
{
    ASSERT(CORE->canvas.font);
    Canvas canvas = CORE->canvas;
    Font *font = CORE->canvas.font;
    CORE->canvas.flags = DrawFlags_Mask;
    CORE->canvas.mask = color;
    i32 columns = font->bitmap.width / font->char_width;
    i32 dx = x;
    i32 dy = y;
    Rect cr;
    char c;
    while (*text) {
        c = *text;
        switch (c) {
            case '\n':
                dx = x;
                dy += font->char_height;
                break;
            default:
                cr.left = (c % columns) * font->char_width;
                cr.top = (c / columns) * font->char_height;
                cr.right = cr.left + font->char_width;
                cr.bottom = cr.top + font->char_height;
                bitmap_draw(&font->bitmap, dx, dy, 0, 0, &cr);
                dx += font->char_width;
                break;
        }
        text++;
    }
    CORE->canvas = canvas;
}

V2
text_draw_attr(const char *text, i32 x, i32 y, TextAttr *attrs, size_t length)
{
    ASSERT(CORE->canvas.font);
    Font *font = CORE->canvas.font;
    Canvas canvas = CORE->canvas;
    CORE->canvas.flags = DrawFlags_Mask;
    i32 columns = font->bitmap.width / font->char_width;
    V2 point;
    point.x = x;
    point.y = y;
    Rect glyph_r, bg_r;

    char glyph;
    while (length) {
        glyph = clamp(*text, 0, 127);
        switch (glyph) {
            case '\n':
                point.x = x;
                point.y += font->char_height;
                break;
            default:
                bg_r = rect_make_size(point.x, point.y, font->char_width, font->char_height);
                rect_draw(bg_r, attrs->bg);

                if (glyph != 0 && glyph != ' ')
                {
                    glyph_r.left = (glyph % columns) * font->char_width;
                    glyph_r.top = (glyph / columns) * font->char_height;
                    glyph_r.right = glyph_r.left + font->char_width;
                    glyph_r.bottom = glyph_r.top + font->char_height;
                    CORE->canvas.mask = attrs->fg;
                    bitmap_draw(&font->bitmap, point.x, point.y, 0, 0, &glyph_r);
                }

                point.x += font->char_width;
                break;
        }
        length--;
        text++;
        attrs++;
    }
    CORE->canvas = canvas;
    return point;
}

//
// Bitmap
//

int
bitmap_init_(Bitmap *bitmap, void *pixels, int type, const char *path)
{
    if (!pixels) {
        return 1;
    }

    printf("Loading bitmap %s\n", path ? path : "unknown path");

    Palette *palette = CORE->palette;

    u32 size = bitmap->width * bitmap->height;
    if (type == PUN_BITMAP_32 || type == PUN_BITMAP_MASK) {
        Color pixel;
        Color *pixels_end = ((Color *)pixels) + size;
        Color *pixels_it = pixels;

        u8 *it = bitmap->pixels;
        int ix;

        if (type != PUN_BITMAP_MASK)
        {
            for (; pixels_it != pixels_end; ++pixels_it) {
                if (pixels_it->a < 0x7F) {
                    ix = 0;
                } else {
                    pixel.r = pixels_it->b;
                    pixel.g = pixels_it->g;
                    pixel.b = pixels_it->r;
                    pixel.a = 0xFF;
                    ix = palette_color_acquire(CORE->palette, pixel, bitmap->palette_range);
                    if (ix < 0) {
                        // printf("unable to acquire color #%02x%02x%02x%02x (%d, %d, %d, %d)%s%s\n",
                        //     pixels_it->r, pixels_it->g, pixels_it->b, pixels_it->a,
                        //     pixels_it->r, pixels_it->g, pixels_it->b, pixels_it->a,
                        //     path ? " in bitmap " : "",
                        //     path ? path : "");
                        ix = palette_color_find_fuzzy(CORE->palette, pixel, bitmap->palette_range);
                        if (ix >= 0) {
                            // printf("- found fuzzy (%d, %d, %d, %d)\n",
                            //     palette->colors[ix].r, palette->colors[ix].g, palette->colors[ix].b,
                            //     palette->colors[ix].a);
                        } else {
                            ix = 0;
                        }
                    }
                }
                *it++ = ix;
            }
        }
        else
        {
            for (; pixels_it != pixels_end; ++pixels_it) {
                *it++ = (pixels_it->a >= 0x7F);
            }
        }

    } else if (type == PUN_BITMAP_8)  {
        memcpy(bitmap->pixels, pixels, size);
    } else {
        // ASSERT_MESSAGE(0, "bitmap_init: Invalid bpp specified.");
        return 0;
    }
    return 1;
}

void
bitmap_init_ex_(Bank *bank, Bitmap *bitmap, i32 width, i32 height, void *pixels, int bpp, int palette_range, const char *path)
{
    bitmap->width = width;
    bitmap->pitch = align_to(width, 16);
    bitmap->height = height;
    bitmap->palette_range = palette_range;

    u32 size = bitmap->pitch * height;
    bitmap->pixels = bank_push(bank, size);
    bitmap_init_(bitmap, pixels, bpp, path);
}

void
bitmap_init(Bitmap *bitmap, i32 width, i32 height, void *pixels, int bpp, int palette_range)
{
    bitmap_init_ex_(CORE->storage, bitmap, width, height, pixels, bpp, palette_range, 0);
}

void
bitmap_clear(Bitmap *bitmap, u8 color)
{
    memset(bitmap->pixels, color, bitmap->width * bitmap->height);
}

#if PUNITY_USE_STB_IMAGE

void
bitmap_load(Bitmap *bitmap, const char *path, int palette_range)
{
    // TODO: Treat path as relative to executable, not the CWD!
    int width, height, comp;
    Color *pixels = (Color *)stbi_load(path, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init_ex_(CORE->storage, bitmap, (i32)width, (i32)height, pixels, PUN_BITMAP_32, palette_range, path);
    free(pixels);
}

void
bitmap_load_resource_ex(Bank *bank, Bitmap *bitmap, const char *resource_name, int palette_range)
{
    size_t size;
    void *ptr = resource_get(resource_name, &size);
    ASSERT(ptr);
    int width, height, comp;
    Color *pixels = (Color *)stbi_load_from_memory(ptr, (int)size, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init_ex_(bank, bitmap, (i32)width, (i32)height, pixels, PUN_BITMAP_32, palette_range, resource_name);
    free(pixels);
}

void
bitmap_load_resource(Bitmap *bitmap, const char *resource_name, int palette_range)
{
    bitmap_load_resource_ex(CORE->storage, bitmap, resource_name, palette_range);
}

void
font_load_resource(Font *font, const char *resource_name, i32 char_width, i32 char_height)
{
    size_t size;
    void *ptr = resource_get(resource_name, &size);
    ASSERT(ptr);
    int width, height, comp;
    Color *pixels = (Color *)stbi_load_from_memory(ptr, (int)size, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init_ex_(CORE->storage, &font->bitmap, (i32)width, (i32)height, pixels, PUN_BITMAP_MASK, 0, resource_name);
    free(pixels);

    font->char_width = char_width;
    font->char_height = char_height;
}

#endif

#if PUNITY_FEATURE_FILEWATCHER

void
filewatcher_init()
{
    FileWatcher *F = &CORE->file_watcher;
    memset(F, 0, sizeof(FileWatcher));
    deque_init(&F->files, 8192);
}

void *
filewatcher_add(const char *path, int path_length, void *extra, size_t extra_size, FileWatcherCallbackF *f)
{
    ASSERT(f);

    FileWatcher *F = &CORE->file_watcher;

    if (path_length == -1) {
        path_length = strlen(path);
        if (path_length >= PUN_PATH_MAX) {
            return 0;
        }
    }
    
    FileWatcherEntry *entry = (FileWatcherEntry*)deque_push(&F->files, sizeof(FileWatcherEntry) + path_length + 1 + extra_size);
    entry->path = (char*)(entry + 1);
    entry->path_length = path_length;
    memcpy(entry->path, path, path_length);
    entry->path[path_length] = 0;
    entry->extra_size = extra_size;
    void *extra_ = entry->path + entry->path_length + 1;
    if (extra) {
        memcpy(extra_, extra, extra_size);
    }
    entry->f = f;

    struct stat file_stat;
    stat(path, &file_stat);
    entry->time = file_stat.st_mtime;

    F->count++;

    return extra_;
}

DEQUE_WALK(filewatcher_check_file_)
{
    FileWatcher *F = (FileWatcher*)user;
    FileWatcherEntry *entry = (FileWatcherEntry*)begin;
    FileWatcherEntry *entry_end = (FileWatcherEntry*)end;
    void *extra;
    struct stat file_stat;
    time_t time;
    while (entry != entry_end) {
        if (entry->f) {
            stat(entry->path, &file_stat);
            if (file_stat.st_mtime != entry->time) {
                extra = entry->path + entry->path_length + 1;
                entry->f(entry->path, entry->path_length, extra);
                entry->time = file_stat.st_mtime;
            }
        } else {
            F->count_inactive++;
        }
        entry = (FileWatcherEntry*)(((uint8_t*)entry) + filewatcherentry_size(entry));
    }

    return 1;
}

void
filewatcher_step()
{
    FileWatcher *F = &CORE->file_watcher;
    if (F->count && F->count != F->count_inactive) {
        F->count_inactive = 0;
        deque_walk(&F->files, filewatcher_check_file_, F);
    }
}

#endif // PUNITY_FEATURE_FILEWATCHER

//
// Keys
//

void
key_clear()
{
    memset(&CORE->key_deltas, 0, PUN_KEYS_MAX);
}

//
// World
//

#if PUNITY_FEATURE_WORLD

//
// State
//

void
state_init(State *S, int state) {
    memset(S, 0, sizeof(State));
    S->next = state;
    S->current = state;
}

void
state_step(State *S)
{
    if (S->next != S->current) {
        S->current = S->next;
        S->next = S->current;
        S->timer = 0;
        S->frame = 0;
    } else {
        S->frame++;
        if (S->timer != 0) {
            S->timer--;
        }
    }
}

//
// World
//

void
scene_init(Scene *S, i32 tile_size, i32 tile_width, i32 tile_height, i32 cell_size)
{
    memset(S, 0, sizeof(Scene));
    S->initialized = 1;
    S->cell_size = cell_size <= 7 ? 32 : cell_size;
    S->entities_count = 0;

    // TODO: Use pow 2 bucket sizes and do & instead of %?
    spatialhash_init(&S->hash, 5003);
    deque_init(&S->deque_entities, sizeof(SceneEntity) * 256);
    // deque_init(&S->deque_tiles, sizeof(WorldTileBlock) * 16);

    if (tile_size)
    {
        usize size = tile_width * tile_height;
        S->tiles = (SceneTile*)virtual_alloc(0, sizeof(SceneTile) * size);
        memset(S->tiles, 0, sizeof(SceneTile) * size);
        S->tile_size = tile_size;
        S->tile_rect.max_x = tile_width;
        S->tile_rect.max_y = tile_height;
    }
}

static inline Rect
scene_cell_range_for_rect(Rect rect, f32 cell_size, Rect *clip)
{
    ASSERT_DEBUG(rect.min_x <= rect.max_x);
    ASSERT_DEBUG(rect.min_y <= rect.max_y);

    Rect result;
    result.min_x = (i32)floorf((f32)(rect.min_x) / cell_size);
    result.min_y = (i32)floorf((f32)(rect.min_y) / cell_size);
    result.max_x = (i32) ceilf((f32)(rect.max_x) / cell_size) + 1;
    result.max_y = (i32) ceilf((f32)(rect.max_y) / cell_size) + 1;

    if (clip) {
        return rect_clip(result, *clip);
    }
    
    return result;
}

SceneTile *
scene_tile(Scene *S, i32 x, i32 y)
{
    if (S->tiles == 0 || x < 0 || y < 0 || x >= S->tile_rect.max_x || y >= S->tile_rect.max_y) {
        return 0;
    }
    return &S->tiles[x + (y * S->tile_rect.max_x)];
}

void
scene_debug_tilemap(Scene *S, u8 color, i32 z)
{
    SceneTile *tile = S->tiles;
    int x, y;
    for (y = 0; y != S->tile_rect.max_y; ++y) {
        for (x = 0; x != S->tile_rect.max_x; ++x) {
            if (tile->flags) {
                frame_draw_push(rect_make_size(
                    x * S->tile_size,
                    y * S->tile_size,
                    S->tile_size,
                    S->tile_size
                ), color, tile->flags, 0, z);
            }
            tile++;
        }
    }
}

void
scene_debug_cells(Scene *S, u8 color, i32 z)
{
    int x, y;
    SpatialCell *cell;

    for (x = 0; x <= (CORE->window.width / S->cell_size); ++x) {
        rect_draw_push(rect_make(x * S->cell_size, 0, (x * S->cell_size) + 1, CORE->window.height), color, z);
    }

    for (y = 0; y <= (CORE->window.height / S->cell_size); ++y) {
        rect_draw_push(rect_make(0, (y * S->cell_size), CORE->window.width, (y * S->cell_size) + 1), color, z);
    }

    char n[16];
    for (y = 0; y != S->hash.rect.max_y; ++y) {
        for (x = 0; x != S->hash.rect.max_x; ++x) {
            cell = spatialhash_get_cell(&S->hash, x, y);
            if (cell) {
                sprintf(n, "%d", cell->items_count);
            } else {
                *n = 0;
            }
            text_draw_push(n, x * S->cell_size + 2, y * S->cell_size + 2, color, z);
        }
    }
}

bool
scene_foreach(Scene *S, Rect rect, SceneForEachCallbackF *callback, void *data, i32 layer)
{
    Rect range;
    i32 cx, cy, ci, tx, ty;
    int flags;

    SceneTile *tile;
    SpatialCell *cell;
    SceneEntity **entity;
    Rect box;
    SceneEntity *tile_entity;

    SceneItem item;
    if (S->tiles)
    {
        item.type = SceneItem_Tile;
        range = scene_cell_range_for_rect(rect, S->tile_size, &S->tile_rect);
        SceneTile *tile = &S->tiles[range.min_y * S->tile_rect.max_x];
        for (cy = range.min_y; cy != range.max_y; ++cy) {
            for (cx = range.min_x; cx != range.max_x; ++cx)
            {
                item.tile = &tile[cx];
                if (item.tile->flags && (layer & item.tile->layer) != 0) {
                    box = rect_make_size(cx * S->tile_size, cy * S->tile_size,
                        S->tile_size, S->tile_size);
                    if (callback(S, &rect, &box, item.tile->flags | EntityFlag_Tile, &item, data)) {
                        return true;
                    }
                }
            }
            tile += S->tile_rect.max_x;
        }
    }

    item.type = SceneItem_Entity;
    range = scene_cell_range_for_rect(rect, S->cell_size, &S->hash.rect);
    for (cy = range.min_y; cy != range.max_y; ++cy) {
        for (cx = range.min_x; cx != range.max_x; ++cx) {
            cell = spatialhash_get_cell(&S->hash, cx, cy);
            while (cell && (cell->x == cx && cell->y == cy)) {
                entity = (SceneEntity**)cell->items;
                for (ci = 0; ci < cell->items_count; ++ci, ++entity)
                {
                    item.entity = *entity;
                    if ((layer & item.entity->layer) != 0 && callback(S, &rect, &item.entity->box, item.entity->flags, &item, data)) {
                        return true;
                    }
                }
                cell = cell->next;
           }
        }
    }

    return false;
}

static inline bool
scene_entity_cast_f_xy_(SceneEntity *A, SceneItem *B, i32 flags,
    Collision *C,
    i32 a_min, i32 a_max,
    i32 b_min, i32 b_max,
    i32 c_min, i32 c_max,
    i32 edge_min, i32 edge_max,
    i32 *m)
{
    i32 mt = (*m);

    // Get the movement distance
    if ((*m) > 0 && c_max >= b_min && (flags & edge_min)) {
        // Moving right.
        mt = b_min - a_max;
    } else if ((*m) < 0 && c_min <= b_max && (flags & edge_max)) {
        // Moving left.
        mt = a_min - b_max;
    }

    if (mt >= 0 && mt < i32_abs(*m)) {
        (*m) = i32_sign(*m) * mt;
        C->B = *B;
    }

    return *m == 0;
}

SCENE_FOREACH_CALLBACK(scene_entity_cast_f_x_)
{
    Collision *C = (Collision *)data;

    if (item->entity == C->A || item_box->max_y <= C->A->box.min_y || item_box->min_y >= C->A->box.max_y) {
        return false;
    }

    return scene_entity_cast_f_xy_(C->A, item, item_flags, C,
        C->A->box.min_x,
        C->A->box.max_x,
        item_box->min_x,
        item_box->max_x,
        C->box.min_x,
        C->box.max_x,
        Edge_Left, Edge_Right,
        &C->mx);
}

SCENE_FOREACH_CALLBACK(scene_entity_cast_f_y_)
{
    Collision *C = (Collision *)data;

    if (item->entity == C->A || item_box->max_x <= C->A->box.min_x || item_box->min_x >= C->A->box.max_x) {
        return false;
    }

    return scene_entity_cast_f_xy_(C->A, item, item_flags, C,
        C->A->box.min_y,
        C->A->box.max_y,
        item_box->min_y,
        item_box->max_y,
        C->box.min_y,
        C->box.max_y,
        Edge_Top, Edge_Bottom,
        &C->my);
}

void
scene_entity_move_(Scene *S, SceneEntity *A, i32 dx, i32 dy)
{
    Rect old_box = A->box;
    rect_tr(&A->box, dx, dy);
    Rect range_old = scene_cell_range_for_rect(old_box, S->cell_size, 0);
    Rect range_new = scene_cell_range_for_rect(A->box,  S->cell_size, 0);
    spatialhash_update(&S->hash, A, range_old, range_new);
}

static Rect
rect_dilate_(Rect rect, i32 mx, i32 my) {
    rect.max_x += i32_positive(mx) * mx;
    rect.min_x += i32_negative(mx) * mx;
    rect.max_y += i32_positive(my) * my;
    rect.min_y += i32_negative(my) * my;
    return rect;
}

// Returns false if entity cannot move full delta (hits an obstacle).
static inline bool
scene_entity_cast_xy_(Scene *S, SceneEntity *A, f32 dx, f32 dy, Collision *C, SceneForEachCallbackF *f)
{
    memset(C, 0, sizeof(Collision));

    C->mx = roundf(dx);
    C->my = roundf(dy);
    
    if (C->mx == 0 && C->my == 0) return true;

    C->A = A;

    C->box = rect_dilate_(A->box, C->mx, C->my);
    // printf("-- foreach\n");
    scene_foreach(S, C->box, f, C, A->layer);

    return C->B.type == SceneItem_None;
}

bool
scene_entity_cast_x(Scene *S, SceneEntity *A, f32 dx, Collision *C)
{
    ASSERT(S->initialized);
    return scene_entity_cast_xy_(S, A, dx, 0, C, scene_entity_cast_f_x_);
}

bool
scene_entity_cast_y(Scene *S, SceneEntity *A, f32 dy, Collision *C)
{
    ASSERT(S->initialized);
    return scene_entity_cast_xy_(S, A, 0, dy, C, scene_entity_cast_f_y_);
}

bool
scene_entity_move_x(Scene *S, SceneEntity *A, f32 dx, Collision *C)
{
    ASSERT(S->initialized);

    A->rx += dx;
    bool r = scene_entity_cast_x(S, A, A->rx, C);
    scene_entity_move_(S, A, C->mx, 0);
    if (!r) {
        // TODO: This is needed only if the collision is stopped here.
        A->rx = 0;
    } else {
        A->rx -= C->mx;
    }

    return r;
}

bool
scene_entity_move_y(Scene *S, SceneEntity *A, f32 dy, Collision *C)
{
    ASSERT(S->initialized);

    A->ry += dy;
    bool r = scene_entity_cast_y(S, A, A->ry, C);
    scene_entity_move_(S, A, 0, C->my);
    if (!r) {
        // TODO: This is needed only if the collision is stopped here.
        A->ry = 0;
    } else {
        A->ry -= C->my;
    }
    // printf("dy %.3f C->my %d A->ry %.3f\n", dy, C->my, A->ry);

    return r;
}

SceneEntity *
scene_entity_add(Scene *S, Rect box, i32 layer)
{
    ASSERT(S->initialized);

    // Allocate from pool.
    SceneEntity *entity = S->entities_pool;
    if (entity) {
        S->entities_pool = entity->next;
    } else {
        entity = deque_push_t(&S->deque_entities, SceneEntity);
    }
    memset(entity, 0, sizeof(SceneEntity));

    entity->next = S->entities;
    S->entities = entity;
    
    entity->box = box;
    entity->flags = Edge_All;
    entity->layer = layer;

    Rect range = scene_cell_range_for_rect(entity->box, S->cell_size, 0);
    spatialhash_add(&S->hash, range, entity);

    return entity;
}

void
scene_entity_remove(Scene *S, SceneEntity *entity)
{
    if (entity->flags & EntityFlag_Removed) {
        ASSERT_MESSAGE(0, "SceneEntity was already removed.");
        return;
    }

    bool removed = false;
    SceneEntity *next = S->entities;
    if (next == entity) {
        S->entities = entity->next;
        removed = true;
    } else {
        while (next) {
            if (next->next == entity) {
                next->next = entity->next;
                removed = true;
                break;
            }
            next = (SceneEntity*)next->next;
        }
    }
    ASSERT(removed);

    entity->next = S->entities_pool;
    entity->flags |= EntityFlag_Removed;
    S->entities_pool = entity;
    S->entities_count--;

    Rect range = scene_cell_range_for_rect(entity->box, S->cell_size, 0);
    spatialhash_remove(&S->hash, range, entity);
}

//
//
//
//
//
// Spatial
//
//
//
//
//

void
spatialhash_init(SpatialHash *H, size_t buckets_count)
{
    ASSERT(buckets_count);

    memset(H, 0, sizeof(SpatialHash));
    H->buckets_count = buckets_count;

    H->buckets = virtual_alloc(0, H->buckets_count * sizeof(SpatialCell*));
    memset(H->buckets, 0, buckets_count * sizeof(SpatialCell*));
    
    deque_init(&H->cells, sizeof(SpatialCell) * 256);
}

void
spatialhash_free(SpatialHash *H)
{
    virtual_free(H->buckets, H->buckets_count * sizeof(SpatialCell*));
    deque_free(&H->cells);
    memset(H, 0, sizeof(SpatialHash));
}

static inline SpatialCell *
spatialhash_new_cell_(SpatialHash *H, i32 x, i32 y, SpatialCell *next)
{
    SpatialCell *cell = H->pool;
    if (cell) {
        H->pool = cell->next;
    } else {
        cell = deque_push_t(&H->cells, SpatialCell);
    }

    cell->items_count = 0;
    cell->x = x;
    cell->y = y;
    cell->next = next;
    return cell;
}

void
spatialhash_add(SpatialHash *H, Rect range, void *item)
{
    H->rect.min_x = minimum(H->rect.min_x, range.min_x);
    H->rect.min_y = minimum(H->rect.min_y, range.min_y);
    H->rect.max_x = maximum(H->rect.max_x, range.max_x);
    H->rect.max_y = maximum(H->rect.max_y, range.max_y);

    size_t bucket_index;
    i32 x, y;
    SpatialCell **bucket;
    SpatialCell *cell;
    for (y = range.min_y; y != range.max_y; ++y) {
        for (x = range.min_x; x != range.max_x; ++x)
        {
            bucket_index = spatialhash_bucket(H, spatialhash_hash(x, y));
            bucket = H->buckets + bucket_index;

            // Find a cell in the bucket at x, y. Skip those that are full.
            cell = *bucket;
            while (cell && (cell->x != x || cell->y != y || cell->items_count == array_count(cell->items))) {
                cell = cell->next;
            }

            if (!cell) {
                // If there's no cell for x and y, we need to add a new one to
                // the start of the bucket.
                cell = *bucket = spatialhash_new_cell_(H, x, y, *bucket);
            } else if (cell->items_count == array_count(cell->items)) {
                // Otherwise, we only add a new cell if the cell we found is already
                // full. We add it before the full one we found.
                cell = spatialhash_new_cell_(H, x, y, cell);
            }

            cell->items[cell->items_count] = item;
            cell->items_count++;
        }
    }
}

static void
spatialhash_remove_(SpatialHash *H, SpatialCell *cell, u32 index, SpatialCell *previous, SpatialCell **bucket)
{
    // static counter = 0;
    // counter++;
    ASSERT(cell->items_count != 0);
    if (cell->items_count != 1) {
        cell->items[index] = cell->items[cell->items_count - 1];
    }
    cell->items_count--;

    if (cell->items_count == 0)
    {
        if (previous)
        {
            previous->next = cell->next;
        }
        else
        {
            // No previous cell, so we're the first cell in the bucket.
            if (cell->next == 0) {
                // No next. Empty the bucket.
                *bucket = 0;
            } else {
                // We have a next cell, so we set that to be the first.
                *bucket = cell->next;
            }
        }
        cell->next = H->pool;
        H->pool = cell;
    }
}

void
spatialhash_remove(SpatialHash *H, Rect range, void *item)
{
    i32 x, y;
    u32 i;
    SpatialCell *cell, *previous, **bucket;
    for (y = range.min_y; y != range.max_y; ++y) {
        for (x = range.min_x; x != range.max_x; ++x)
        {
            bucket = H->buckets + spatialhash_bucket(H, spatialhash_hash(x, y));
            cell = *bucket;
            previous = 0;
            while (cell && (cell->x != x || cell->y != y)) {
                previous = cell;
                cell = cell->next;
            }

            if (cell)
            {
                do {
                    for (i = 0; i != cell->items_count; ++i) {
                        if (cell->items[i] == item) {
                            spatialhash_remove_(H, cell, i, previous, bucket);
                            goto next;
                        }
                    }
                    cell = cell->next;
                } while (cell && (cell->x == x && cell->y == y));
            }
next:;
        }
    }
}

void
spatialhash_update(SpatialHash *H, void *item, Rect old_range, Rect new_range)
{
    if ((old_range.min_x != new_range.min_x) ||
        (old_range.min_y != new_range.min_y) ||
        (old_range.max_x != new_range.max_x) ||
        (old_range.max_y != new_range.max_y))
    {
        // TODO: Is it worth to optimize this?
        spatialhash_remove(H, old_range, item);
        spatialhash_add(H, new_range, item);
    }
}

SpatialCell *
spatialhash_get_cell(SpatialHash *H, int x, int y)
{
    if (x >= H->rect.min_x && x < H->rect.max_x &&
        y >= H->rect.min_y && y < H->rect.max_y)
    {
        SpatialCell **bucket = H->buckets + spatialhash_bucket(H, spatialhash_hash(x, y));

        // Find a cell in the bucket at x, y that is not full.
        SpatialCell *cell = *bucket;
        while (cell && (cell->x != x || cell->y != y)) {
            cell = cell->next;
        }
        return cell;
    }
    return 0;
}

#endif // PUNITY_FEATURE_WORLD

//
// Sound
//

#if PUNITY_USE_STB_VORBIS

static void
sound_load_stbv_(Sound *sound, stb_vorbis *stream)
{
    stb_vorbis_info info = stb_vorbis_get_info(stream);
    sound->volume = PUNP_SOUND_DEFAULT_SOUND_VOLUME;
    sound->rate = info.sample_rate;
    sound->channels = info.channels;
    sound->samples_count = stb_vorbis_stream_length_in_samples(stream);
    sound->samples = bank_push(CORE->storage, PUNP_SOUND_SAMPLES_TO_BYTES(sound->samples_count, sound->channels));

    static i16 buffer[1024];
    i16 *it = sound->samples;
    int samples_read_per_channel;
    for (;;) {
        samples_read_per_channel =
            stb_vorbis_get_samples_short_interleaved(stream, PUNP_SOUND_CHANNELS, buffer, 1024);

        if (samples_read_per_channel == 0) {
            break;
        }

        // 2 channels, 16 bits per sample.
        memcpy(it, buffer, PUNP_SOUND_SAMPLES_TO_BYTES(samples_read_per_channel, sound->channels));
        it += samples_read_per_channel * sound->channels;
    }

    stb_vorbis_close(stream);
}

void
sound_load(Sound *sound, const char *path)
{
    memset(sound, 0, sizeof(Sound));

    int error = 0;
    stb_vorbis *stream = stb_vorbis_open_filename(path, &error, 0);
    ASSERT(!error && stream);

    sound_load_stbv_(sound, stream);

    size_t size = strlen(path) + 1;
    sound->name = bank_push(CORE->storage, size);
    memcpy(sound->name, path, size);
}

void
sound_load_resource(Sound *sound, const char *resource_name)
{
    memset(sound, 0, sizeof(Sound));

    size_t resource_size;
    void *resource_data = resource_get(resource_name, &resource_size);
    ASSERT(resource_data);

    int error = 0;
    stb_vorbis *stream =
            stb_vorbis_open_memory(resource_data, resource_size,
                                   &error, 0);

    ASSERT(!error && stream);

    sound_load_stbv_(sound, stream);

    size_t size = strlen(resource_name) + 1;
    sound->name = bank_push(CORE->storage, size);
    memcpy(sound->name, resource_name, size);
}

#endif // PUNITY_USE_STB_VORBIS

typedef struct PunPAudioSource
{
    Sound *sound;
    f32 rate;
    size_t position;
    struct PunPAudioSource *next;
}
PunPAudioSource;

static PunPAudioSource *punp_audio_source_pool = 0;
static PunPAudioSource *punp_audio_source_playback = 0;

void
sound_play(Sound *sound)
{
    if (sound->sources_count > 3) {
        printf("Too many same sounds playing at once.\n");
        return;
    }
    PunPAudioSource *source = 0;
    if (punp_audio_source_pool == 0) {
        // Pool is empty, therefore we must allocate a new source.
        source = bank_push(CORE->storage, sizeof(PunPAudioSource));
        // printf("Allocating audio source.\n");
    } else {
        // We have something in the pool.
        source = punp_audio_source_pool;
        punp_audio_source_pool = source->next;
        // printf("Pooling audio source.\n");
    }

    if (source)
    {
        memset(source, 0, sizeof(PunPAudioSource));
        sound->sources_count++;
        source->sound = sound;
        source->rate = (f32)sound->rate / (f32)PUNITY_SOUND_SAMPLE_RATE;
        source->next = punp_audio_source_playback;
        punp_audio_source_playback = source;
    }
}

// Called from platform to fill in the audio buffer.
//
static void
sound_mix_(i16 *buffer, size_t samples_count)
{
    BankState bank_state = bank_begin(CORE->stack);

    size_t size =  samples_count * sizeof(f32);
    f32 *channel0 = bank_push(CORE->stack, size);
    f32 *channel1 = bank_push(CORE->stack, size);

    f32 *it0;
    f32 *it1;
    size_t i;

    //
    // Zero the channels.
    //

    it0 = channel0;
    it1 = channel1;

    memset(it0, 0, size);
    memset(it1, 0, size);

    //
    // Mix the sources.
    //

    size_t sound_samples;
    size_t sound_samples_remaining;
    size_t loop_samples;
    Sound *sound = 0;
    PunPAudioSource **it = &punp_audio_source_playback;
    PunPAudioSource *source;

    while (*it)
    {
        source = *it;

        i16 *sample;
        it0 = channel0;
        it1 = channel1;

        sound = source->sound;

        loop_samples = samples_count;

loop:;
        sound_samples = loop_samples;
        sound_samples_remaining = (sound->samples_count - source->position) / source->rate;
        if (sound_samples > sound_samples_remaining) {
            sound_samples = sound_samples_remaining;
        }

        if (sound->channels == 2)
        {
            for (i = 0; i != sound_samples; ++i)
            {
                sample = &sound->samples[(source->position + (size_t)((f32)i * source->rate)) * 2];
                //sample = punp_audio_source_sample(source, i);
                *it0++ += sample[0] * sound->volume;
                *it1++ += sample[1] * sound->volume;
            }
        }
        else if (sound->channels == 1)
        {
            for (i = 0; i != sound_samples; ++i)
            {
                sample = &sound->samples[(source->position + (size_t)((f32)i * source->rate))];
                //sample = punp_audio_source_sample(source, i);
                *it0++ += sample[0] * sound->volume;
                *it1++ += sample[0] * sound->volume;
            }
        }
        else
        {
            ASSERT_MESSAGE(0, "Invalid count of channels (2 or 1 are supported).");
        }

        source->position += maximum(1, sound_samples * source->rate);
        
        ASSERT(source->position <= sound->samples_count);
        if (source->position == sound->samples_count)
        {
            if (sound->flags & SoundFlag_Loop) {
                source->position = 0;
                ASSERT(loop_samples >= sound_samples);
                loop_samples -= sound_samples;
                if (loop_samples != 0) {
                    goto loop;
                }
            } else {
                *it = source->next;
                sound->sources_count--;
                source->next = punp_audio_source_pool;
                punp_audio_source_pool = source;
            }
        }
        else
        {
            it = &source->next;
        }
    }

    //
    // Put to output buffer, clamp and convert to 16-bit.
    //

    it0 = channel0;
    it1 = channel1;
    f32 s1, s2;
    i16 *it_buffer = buffer;
    for (i = 0; i != samples_count; ++i)
    {
        s1 = *it0++ * CORE->audio_volume;
        s2 = *it1++ * CORE->audio_volume;
        *it_buffer++ = (i16)clamp(s1, -32768, 32767);
        *it_buffer++ = (i16)clamp(s2, -32768, 32767);
    }
// #undef MIX_CLIP_

    bank_end(&bank_state);
}

//
//
//

#if PUNITY_LIB == 0

// Some platform tests to make sure some special tricks in the lib work correctly.
void
punity_test()
{
    ASSERT(i32_sign(-1) == -1);
    ASSERT(i32_sign(+1) == +1);
    ASSERT(i32_sign(0) == 0);

    ASSERT(i32_abs(-1) == 1);
    ASSERT(i32_abs(+1) == 1);
    ASSERT(i32_abs(0) ==  0);

    ASSERT(i32_positive(+1) == 1);
    ASSERT(i32_positive(0)  == 1);
    ASSERT(i32_positive(-1) == 0);

    ASSERT(i32_negative(+1) == 0);
    ASSERT(i32_negative(0)  == 0);
    ASSERT(i32_negative(-1) == 1);
}

int
punity_init(const char *args)
{
#if PUNITY_CONSOLE
    // printf("Debug build...");
    // if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#else
    // printf("Release build...");
    // FreeConsole();
    // freopen("CONOUT$", "w", stdout);
    // freopen("CONOUT$", "w", stderr);
#endif

#if PUNITY_SIMD
    simd_init__();
#endif

    punity_test();

    static Bank s_stack = {0};
    static Bank s_storage = {0};
    
    static Core s_core = {0};
    CORE = &s_core;
    memset(CORE, 0, sizeof(Core));

    static Palette s_palette;
    CORE->palette = &s_palette;
    palette_init(CORE->palette);
    palette_color_set(CORE->palette, PUN_COLOR_TRANSPARENT, color_make(0x00, 0x00, 0x00, 0x00));
    palette_color_set(CORE->palette, 1, color_make(0x00, 0x00, 0x00, 0xFF));
    palette_color_set(CORE->palette, 2, color_make(0xFF, 0xFF, 0xFF, 0xFF));

    CORE->args = args;
    CORE->window.width  = 320;
    CORE->window.height = 200;
    CORE->window.scale  = 3;

    // configure(&CORE->config);

    CORE->running = 1;
    CORE->stack = &s_stack;
    CORE->storage = &s_storage;

    bank_init(CORE->stack, PUNITY_STACK_CAPACITY);
    bank_init(CORE->storage, PUNITY_STORAGE_CAPACITY);

    static DrawList s_draw_list = {0};
    CORE->draw_list = &s_draw_list;
    drawlist_init(CORE->draw_list, PUNITY_DRAW_LIST_RESERVE);

    CORE->audio_volume = PUNP_SOUND_DEFAULT_MASTER_VOLUME;

    CORE->background = color_make(0x00, 0x00, 0x00, 0x00);

#if PUNITY_FEATURE_FILEWATCHER
    filewatcher_init();
#endif

    if (!init()) {
        return 1;
    }

    static Bitmap s_canvas_bitmap = { 0 };
    CORE->canvas.bitmap = &s_canvas_bitmap;
    bitmap_init(CORE->canvas.bitmap, CORE->window.width, CORE->window.height, 0, 0, 0);
    bitmap_clear(CORE->canvas.bitmap, PUN_COLOR_TRANSPARENT);
    CORE->window.buffer = CORE->canvas.bitmap->pixels;
    clip_reset();

    return 0;
}

void
punity_frame_begin()
{
    key_clear();
    perf_from(&CORE->perf_frame);
}

void
punity_frame_end()
{
    CORE->time_delta = perf_delta(&CORE->perf_frame);
    CORE->frame++;
    CORE->time += CORE->time_delta;
    CORE->key_text_length = 0;
    CORE->key_text[0] = 0;

    bool throttle = CORE->recorder.active;
#if !PUNITY_OPENGL || PUNITY_OPENGL_30FPS
    throttle = true;
#endif
    if (throttle && CORE->time_delta < (1.0f/30.0f)) {
        int sleep = ((1.0f/30.0f) - CORE->time_delta) * 1e3;
        Sleep(sleep);
        CORE->time_delta = (1.0f/30.0f);
    }
}

#if PUNITY_FEATURE_RECORDER
void record_frame_();
#endif

void
punity_frame_step()
{
#if PUNITY_FEATURE_FILEWATCHER
    filewatcher_step();
#endif
    
    BankState stack_state = bank_begin(CORE->stack);
    perf_from(&CORE->perf_step);
    drawlist_begin(CORE->draw_list);
    step();
    drawlist_end(CORE->draw_list);
    drawlist_clear(CORE->draw_list);
    perf_to(&CORE->perf_step);
    bank_end(&stack_state);

#if PUNITY_FEATURE_RECORDER
    record_frame_();
    if (key_pressed(PUNITY_FEATURE_RECORDER_KEY)) {
        if (CORE->recorder.active) {
            record_end();
        } else {
            record_begin();
        }
    }
#endif
}

void
punity_on_key_down(u8 key, u8 modifiers)
{
    CORE->key_modifiers = modifiers;
    if (key < PUN_KEYS_MAX) {
        CORE->key_states[key] = 1;
        CORE->key_deltas[key] = 1;
    }
}

void
punity_on_key_up(u8 key, u8 modifiers)
{
    CORE->key_modifiers = modifiers;
    if (key < PUN_KEYS_MAX) {
        CORE->key_states[key] = 0;
        CORE->key_deltas[key] = 1;
    }
}

void
punity_on_key_up_all(u8 modifiers)
{
    CORE->key_modifiers = modifiers;
    for (int key = 0; key != PUN_KEYS_MAX; key++) {
        CORE->key_deltas[key] = CORE->key_states[key];
        CORE->key_states[key] = 0;
    }
}

void
punity_on_char(u32 ch)
{
    if (CORE->key_text_length != array_count(CORE->key_text) && ch >= ' ') {
#if PUNITY_TEXT_UNICODE
        CORE->key_text[CORE->key_text_length] = (u32)ch;
#else
        CORE->key_text[CORE->key_text_length] = (char)ch;
#endif
        CORE->key_text_length++;
    }
}

void
punity_on_text(const char *text)
{
    ASSERT(text);
    usize length = strlen(text);
    if (length > array_count(CORE->key_text)) {
        printf("Warning: Text buffer passed is too long (%d bytes but only %d expected).",
            length,
            array_count(CORE->key_text));
    }
    CORE->key_text_length = minimum(length, array_count(CORE->key_text));
    memcpy(CORE->key_text, text, CORE->key_text_length);
}

//
// Recorder.
//

#if PUNITY_FEATURE_RECORDER
void
record_begin()
{
    Recorder *R = &CORE->recorder;
    R->active = true;
    size_t width  = CORE->window.width;
    size_t height = CORE->window.height;
    size_t block_size = sizeof(RecorderFrameBlock) + (width * height * PUN_RECORDER_FRAMES_PER_BLOCK);
    if (block_size != R->frames.block_size) {
        deque_free(&R->frames);
        deque_init(&R->frames, block_size);
    } else {
        deque_clear(&R->frames);
    }
}

void
record_frame_()
{
    Recorder *R = &CORE->recorder;
    if (!R->active) {
        return;
    }

    isize frame_size = CORE->window.width * CORE->window.height;

    RecorderFrame *frame;
    RecorderFrameBlock *last = 0;
    if (R->frames.last) {
        last = (RecorderFrameBlock*)R->frames.last->begin;
    }
    if (!last || last->frames_count == PUN_RECORDER_FRAMES_PER_BLOCK)
    {
        last = deque_block_push_t(&R->frames, RecorderFrameBlock);
        last->frames_count = 0;
        last->pixels = (uint8_t*)(last + 1);

        uint8_t *pixels_it = last->pixels;
        frame = last->frames;
        for (size_t i = 0;
             i != PUN_RECORDER_FRAMES_PER_BLOCK;
             ++i, ++frame, pixels_it += frame_size)
        {
            frame->pixels = pixels_it;
        }
    }

    frame = last->frames + last->frames_count;
    ++last->frames_count;
    ++R->frames_count;

    frame->color_table.count = array_count(CORE->palette->colors);
    for (int i = 0; i != frame->color_table.count; ++i) {
        frame->color_table.colors[i].r = CORE->palette->colors[i].r;
        frame->color_table.colors[i].g = CORE->palette->colors[i].g;
        frame->color_table.colors[i].b = CORE->palette->colors[i].b;
    }

    memcpy(frame->pixels, CORE->canvas.bitmap->pixels, frame_size);
}

GIFW_WRITE_CALLBACK(record_write_data_)
{
    FILE *file = (FILE*)user;
    size_t w = fwrite(begin, 1, end - begin, file);
    if (w != (end - begin)) {
        printf("Unable to write to file.");
        return 0;
    }
    return 1;
}

DEQUE_WALK(record_write_frames_)
{
    Recorder *R = (Recorder*)user;
    RecorderFrameBlock *block = (RecorderFrameBlock*)begin;
    RecorderFrame *frame = block->frames;
    for (size_t i = 0; i != block->frames_count; ++i, ++frame) {
        printf("- %d%%\n", progress);
        gifw_frame(&R->gif,
            frame->pixels,
            &frame->color_table,
            0, 0, R->gif.width, R->gif.height,
            (R->gif.frames_count % 3) == 0 ? 4 : 3,
            GIFWFrameDispose_NotSpecified,
            0, 0
        );
    }

    return 1;
}

void
record_end()
{
    Recorder *R = &CORE->recorder;
    if (!R->active) {
        return;
    }
    R->active = false;

    printf("Writing GIF...\n");
    FILE *file = fopen("record.gif", "wb");
    if (file) {
        gifw_begin(&R->gif,
            // Size
            CORE->window.width, CORE->window.height,
            // Repeat
            0,
            // Color table, background
            0, 0,
            // Callback
            record_write_data_, file
        );

        deque_walk(&R->frames, record_write_frames_, R);
        gifw_end(&R->gif);

        fclose(file);
    }
}

#else
void record_begin() {};
void record_end() {};
#endif

#endif // if PUNITY_LIB == 0

#if PUN_RUNTIME_SDL
#include "punity-sdl.c"
#else

#if PUNITY_LIB == 0
//
// Windows
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#if PUN_RUNTIME_WINDOWS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static struct
{
    HINSTANCE instance;
    HCURSOR cursor;
    HWND window;
    u32 *window_buffer;
    WINDOWPLACEMENT fullscreen_placement;

    // Initial properties for window
    char window_init_title[512];
    int  window_init_flags;

    LPDIRECTSOUND8 direct_sound;
    LPDIRECTSOUNDBUFFER audio_buffer;
    WAVEFORMATEX audio_format;
    DSBUFFERDESC audio_buffer_description;
    DWORD audio_cursor;

#if PUNITY_OPENGL
    int gl_texture;
#endif
}
win32_ = {0};

#define WIN32_DEFAULT_STYLE_ (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#if PUNITY_OPENGL

#include <GL/gl.h>
#pragma comment(lib, "opengl32.lib")

#define GL_BGRA 0x80E1

#define PUN_GL_DECL WINAPI
typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define PUN_GL_LIST_WIN32 \
    GL_F(void,      BlendEquation,           GLenum mode) \
    GL_F(void,      ActiveTexture,           GLenum texture) \

typedef BOOL WINAPI wglChoosePixelFormatARBF(HDC hdc,
    const int *piAttribIList,
    const FLOAT *pfAttribFList,
    UINT nMaxFormats,
    int *piFormats,
    UINT *nNumFormats);

typedef HGLRC WINAPI wglCreateContextAttribsARBF(HDC hDC,
    HGLRC hShareContext,
    const int *attribList);

extern wglChoosePixelFormatARBF *wglChoosePixelFormatARB = 0;
extern wglCreateContextAttribsARBF *wglCreateContextAttribsARB = 0;

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_FULL_ACCELERATION_ARB               0x2027
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

// #define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
// #define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
// #define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001


// Common functions.
#define PUN_GL_LIST

#define GL_F(ret, name, ...) typedef ret PUN_GL_DECL name##proc(__VA_ARGS__); name##proc * gl##name;
PUN_GL_LIST
PUN_GL_LIST_WIN32
#undef GL_F

int win32_gl_window_set_pixel_format_(HDC dc);

int
win32_gl_init_()
{
    WNDCLASSA wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = win32_.instance;
    wc.lpszClassName = "PunityWGLLoaderClass";
    if (!RegisterClassA(&wc)) {
        LOG("RegisterClassA failed.");
        return 0;
    }

    HWND window = CreateWindowExA(0, wc.lpszClassName, "PunityWGLLoader",
        0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, wc.hInstance, 0);
    if (!window) {
        LOG("CreateWindowExA failed.");
        return 0;
    }

    HDC dc = GetDC(window);
    win32_gl_window_set_pixel_format_(dc);
    HGLRC glc = wglCreateContext(dc);
    if (!wglMakeCurrent(dc, glc)) {
        LOG("wglMakeCurrent failed.");
        return 0;
    }

    HINSTANCE dll = LoadLibraryA("opengl32.dll");
    if (!dll) {
        LOG("LoadLibraryA('opengl32.dll') failed.");
        return 0;
    }

    typedef PROC WINAPI wglGetProcAddressF(LPCSTR lpszProc);

    wglGetProcAddressF* wglGetProcAddress =
        (wglGetProcAddressF*)GetProcAddress(dll, "wglGetProcAddress");
    wglChoosePixelFormatARB =
        (wglChoosePixelFormatARBF *)wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB =
       (wglCreateContextAttribsARBF *)wglGetProcAddress("wglCreateContextAttribsARB");

#define GL_F(ret, name, ...) \
            gl##name = (name##proc *)wglGetProcAddress("gl" #name); \
            if (!gl##name) { \
                FAIL("Function gl" #name " couldn't be loaded from opengl32.dll\n"); \
                goto end; \
            }
        PUN_GL_LIST
        PUN_GL_LIST_WIN32
#undef GL_F
end:

    wglMakeCurrent(0, 0);
    wglDeleteContext(glc);
    ReleaseDC(window, dc);
    DestroyWindow(window);

    return 1;
}

int
win32_gl_window_set_pixel_format_(HDC dc)
{
    int pixel_format = 0;
    int extended_pixel_format = 0;
    if (wglChoosePixelFormatARB)
    {
        int attrib[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };

        // if (!OpenGLSupportsSRGBFramebuffer) {
        //     attrib[10] = 0;
        // }

        wglChoosePixelFormatARB(dc, attrib, 0, 1,
            &pixel_format, &extended_pixel_format);
    }

    if (!extended_pixel_format)
    {
        PIXELFORMATDESCRIPTOR desired_pixel_format_desc = { 0 };
        // TODO: Program seems to work perfectly fine without all other params except dwFlags.
        //       Can we skip other params for the sake of brevity?
        desired_pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        desired_pixel_format_desc.nVersion = 1;
        // PFD_DOUBLEBUFFER appears to prevent OBS from reliably streaming the window
        desired_pixel_format_desc.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        desired_pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format_desc.cColorBits = 32;
        desired_pixel_format_desc.cAlphaBits = 8;
        desired_pixel_format_desc.cDepthBits = 32;
        desired_pixel_format_desc.dwLayerMask = PFD_MAIN_PLANE;
        //

        pixel_format = ChoosePixelFormat(dc, &desired_pixel_format_desc);
        if (!pixel_format) {
            FAIL("ChoosePixelFormat failed");
        }
        
    }

    // Technically you do not need to call DescribePixelFormat here,
    // as SetPixelFormat doesn't actually need it to be filled out properly.
    PIXELFORMATDESCRIPTOR pixel_format_desc;
    DescribePixelFormat(dc, pixel_format, sizeof(pixel_format_desc), &pixel_format_desc);
    if (!SetPixelFormat(dc, pixel_format, &pixel_format_desc)) {
        FAIL("SetPixelFormat failed");
    }

    return 1;
}

int
win32_gl_window_init_(HWND hwnd)
{
    HDC dc = GetDC(hwnd);
    win32_gl_window_set_pixel_format_(dc);

    static int win32_gl_attribs_[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        WGL_CONTEXT_FLAGS_ARB, 0
#if PUNITY_DEBUG
        | WGL_CONTEXT_DEBUG_BIT_ARB
#endif
        ,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0,
    };

    HGLRC glc = 0;
    if(wglCreateContextAttribsARB) {
        glc = wglCreateContextAttribsARB(dc, 0, win32_gl_attribs_);
        // TODO: Fallback to glc = wglCreateContext(dc)?
    } else {
        FAIL("wglCreateContextAttribsARB not available");
    }

    if (!glc) {
        FAIL("wglCreateContextAttribsARB failed");
    }

    if (!wglMakeCurrent(dc, glc)) {
        FAIL("wglMakeCurrent failed");
    }

    ReleaseDC(hwnd, dc);

    return 1;
}

#endif // PUNITY_OPENGL

//
// Sound
//

typedef HRESULT(WINAPI*DirectSoundCreate8F)(LPGUID,LPDIRECTSOUND8*,LPUNKNOWN);

static bool
win32_sound_init_(HWND window)
{
    HRESULT hr;

    HMODULE dsound_lib = LoadLibraryA("dsound.dll");
    if (!dsound_lib) {
        printf("LoadLibrary(dsound.dll) failed.\n");
        return 0;
    }

    DirectSoundCreate8F DirectSoundCreate8 =
            (DirectSoundCreate8F)GetProcAddress(dsound_lib, "DirectSoundCreate8");
    if (!DirectSoundCreate8) {
        printf("GetProcAddress(DirectSoundCreate8) failed.\n");
        return 0;
    }

    hr = DirectSoundCreate8(0, &win32_.direct_sound, 0);
    if (hr != DS_OK) {
        printf("DirectSoundCreate failed.\n");
        return 0;
    }

    hr = IDirectSound8_SetCooperativeLevel(win32_.direct_sound,
        window,
        DSSCL_PRIORITY);
    if (hr != DS_OK) {
        printf("win32_.direct_sound->SetCooperativeLevel failed.\n");
        return 0;
    }

    DSBUFFERDESC primary_buffer_description = {0};
    primary_buffer_description.dwSize = sizeof(primary_buffer_description);
    primary_buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

    LPDIRECTSOUNDBUFFER primary_buffer;
    hr = IDirectSound8_CreateSoundBuffer(win32_.direct_sound, &primary_buffer_description, &primary_buffer, 0);
    if (hr != DS_OK) {
        printf("win32_.direct_sound->CreateSoundBuffer for primary buffer failed.\n");
        return 0;
    }

    win32_.audio_format.wFormatTag = WAVE_FORMAT_PCM;
    win32_.audio_format.nChannels = PUNP_SOUND_CHANNELS;
    win32_.audio_format.nSamplesPerSec = PUNITY_SOUND_SAMPLE_RATE;
    win32_.audio_format.wBitsPerSample = 16;
    win32_.audio_format.nBlockAlign = (win32_.audio_format.nChannels * win32_.audio_format.wBitsPerSample) / 8;
    win32_.audio_format.nAvgBytesPerSec = win32_.audio_format.nSamplesPerSec * win32_.audio_format.nBlockAlign;
    win32_.audio_format.cbSize = 0;

    hr = IDirectSoundBuffer8_SetFormat(primary_buffer, &win32_.audio_format);
    if (hr != DS_OK) {
        printf("primary_buffer->SetFormat failed.");
        return 0;
    }

    // DSBSIZE_MIN DSBSIZE_MAX

    win32_.audio_buffer_description.dwSize = sizeof(win32_.audio_buffer_description);
    // 2 seconds.
    win32_.audio_buffer_description.dwBufferBytes = PUNP_SOUND_BUFFER_BYTES;
    win32_.audio_buffer_description.lpwfxFormat = &win32_.audio_format;
    // dicates that IDirectSoundBuffer::GetCurrentPosition should use the new behavior of the play cursor.
    // In DirectSound in DirectX 1, the play cursor was significantly ahead of the actual playing sound on
    // emulated sound cards; it was directly behind the write cursor.
    // Now, if the DSBCAPS_GETCURRENTPOSITION2 flag is specified, the application can get a more accurate
    // play position. If this flag is not specified, the old behavior is preserved for compatibility.
    // Note that this flag affects only emulated sound cards; if a DirectSound driver is present, the play
    // cursor is accurate for DirectSound in all versions of DirectX.
    win32_.audio_buffer_description.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    
    hr = IDirectSound8_CreateSoundBuffer(win32_.direct_sound,
                                         &win32_.audio_buffer_description,
                                         &win32_.audio_buffer,
                                         0);
    if (hr != DS_OK)
    {
        printf("win32_.direct_sound->CreateSoundBuffer for secondary buffer failed.\n");
        return 0;
    }

    // IDirectSoundBuffer8_SetFormat(win32_.audio_buffer, &win32_.audio_format);

    // Clear the initial buffer.

    LPVOID region1, region2;
    DWORD region1_size, region2_size;

    hr = IDirectSoundBuffer8_Lock(win32_.audio_buffer,
                                  0, win32_.audio_buffer_description.dwBufferBytes,
                                  (LPVOID *)&region1, &region1_size,
                                  (LPVOID *)&region2, &region2_size,
                                  DSBLOCK_ENTIREBUFFER);

    if (hr == DS_OK)
    {
        memset(region1, 0, region1_size);
        IDirectSoundBuffer8_Unlock(win32_.audio_buffer,
                                   region1, region1_size,
                                   region2, region2_size);
    }

    hr = IDirectSoundBuffer8_Play(win32_.audio_buffer, 0, 0, DSBPLAY_LOOPING);

    if (hr != DS_OK)
    {
        ASSERT(0);
    }
    return 1;
}

static void
win32_sound_step_()
{
//  if (!punp_audio_source_playback) {
//      return;
//  }

    HRESULT hr;

    DWORD cursor_play, cursor_write;
    hr = IDirectSoundBuffer8_GetCurrentPosition(win32_.audio_buffer,
                                                &cursor_play, &cursor_write);

    if (FAILED(hr)) {
        printf("IDirectSoundBuffer8_GetCurrentPosition failed.\n");
        return;
    }

    u32 chunk_size = PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_SAMPLES, PUNP_SOUND_CHANNELS);
    u32 chunk = cursor_write / chunk_size;

    DWORD lock_cursor = ((chunk+1) * chunk_size) % PUNP_SOUND_BUFFER_BYTES;
    DWORD lock_size = chunk_size;

    if (lock_cursor == win32_.audio_cursor) {
        return;
    }

    VOID *range1, *range2;
    DWORD range1_size, range2_size;
    hr = IDirectSoundBuffer8_Lock(win32_.audio_buffer,
                                  lock_cursor,
                                  lock_size,
                                  &range1, &range1_size,
                                  &range2, &range2_size,
                                  0);
    if (hr != DS_OK) {
        printf("IDirectSoundBuffer8_Lock failed.\n");
        return;
    }

    sound_mix_(range1, PUNP_SOUND_BYTES_TO_SAMPLES(range1_size, PUNP_SOUND_CHANNELS));
    if (range2) {
        sound_mix_(range2, PUNP_SOUND_BYTES_TO_SAMPLES(range2_size, PUNP_SOUND_CHANNELS));
    }

    IDirectSoundBuffer8_Unlock(win32_.audio_buffer,
                               range1, range1_size,
                               range2, range2_size);

    win32_.audio_cursor = lock_cursor;
}

//
//
//

void
window_title(const char *title)
{
    if (win32_.window) {
        SetWindowTextA(win32_.window, title);
    } else {
        int length = minimum(array_count(win32_.window_init_title) - 1, strlen(title));
        memcpy(win32_.window_init_title, title, length);
        win32_.window_init_title[length] = 0;
    }
}

bool
window_is_fullscreen()
{
    if (win32_.window) {
        DWORD style = GetWindowLong(win32_.window, GWL_STYLE);
        int normal = style & WIN32_DEFAULT_STYLE_;
        return !normal;
    } else {
        return win32_.window_init_flags;
    }
}

void
window_fullscreen_toggle()
{
    if (!win32_.window) {
        win32_.window_init_flags = !win32_.window_init_flags;
        return;
    }

    DWORD style = GetWindowLong(win32_.window, GWL_STYLE);
    int normal = style & WIN32_DEFAULT_STYLE_;
    if (normal)
    {
        // Set fullscreen.
        MONITORINFO monitor_info = { sizeof(MONITORINFO) };
        if (GetWindowPlacement(win32_.window, &win32_.fullscreen_placement) &&
            GetMonitorInfo(MonitorFromWindow(win32_.window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
        {
            SetWindowLong(win32_.window, GWL_STYLE, style & ~WIN32_DEFAULT_STYLE_);
            SetWindowPos(win32_.window, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        // TODO: Check if we have `win32_.fullscreen_placement` stored,
        //       otherwise we're in a fake full-screen.
        SetWindowLong(win32_.window, GWL_STYLE, style | WIN32_DEFAULT_STYLE_);
        SetWindowPlacement(win32_.window, &win32_.fullscreen_placement);
        SetWindowPos(win32_.window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

//
// Input
//

static u32
win32_app_key_mods_()
{
    u32 mods = 0;

    if (GetKeyState(VK_SHIFT) & (1 << 31))
        mods |= KEY_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & (1 << 31))
        mods |= KEY_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & (1 << 31))
        mods |= KEY_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1 << 31))
        mods |= KEY_MOD_SUPER;

    return mods;
}

static LRESULT CALLBACK
win32_window_callback_(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message) {
        case WM_SETCURSOR: {
            if (LOWORD(lp) == HTCLIENT) {
                SetCursor(0);
                return TRUE;
            }
            // TODO: Cursor is not shown when mouse is over the non-client area.
            // Pass to DefWindowProc.
        }
        break;

        case WM_KEYDOWN: {
            if (!((lp >> 30) & 1)) {
                // if (wp >= 'a' && wp <= 'z') {
                //     wp -= 'a' - 'A';
                // }
                punity_on_key_down(wp, win32_app_key_mods_());
            }
            return 0;
        }
        break;

        case WM_KEYUP: {
            // if (wp >= 'a' && wp <= 'z') {
            //     wp -= 'a' - 'A';
            // }
            punity_on_key_up(wp, win32_app_key_mods_());
            return 0;
        }
        break;

        case WM_LBUTTONDOWN: { punity_on_key_down(VK_LBUTTON, win32_app_key_mods_()); return 0; } break;
        case WM_LBUTTONUP:   { punity_on_key_up(VK_LBUTTON, win32_app_key_mods_()); return 0; } break;
        case WM_RBUTTONDOWN: { punity_on_key_down(VK_RBUTTON, win32_app_key_mods_()); return 0; } break;
        case WM_RBUTTONUP:   { punity_on_key_up(VK_RBUTTON, win32_app_key_mods_()); return 0; } break;
        case WM_MBUTTONDOWN: { punity_on_key_down(VK_MBUTTON, win32_app_key_mods_()); return 0; } break;
        case WM_MBUTTONUP:   { punity_on_key_up(VK_MBUTTON, win32_app_key_mods_()); return 0; } break;

        case WM_CHAR: {
            punity_on_char(wp);
            return 0;
        }

        case WM_CLOSE: {
            PostQuitMessage(0);
            CORE->running = 0;
        }
        break;

        case WM_KILLFOCUS: {
            punity_on_key_up_all(win32_app_key_mods_());
        }
        break;

        case WM_SIZE:
        {
            f32 width  = (UINT)(lp & 0xffff);
            f32 height = (UINT)(lp >> 16);
            
            f32 scale_x = width  / CORE->window.width;
            f32 scale_y = height / CORE->window.height;
            
            CORE->window.scale = maximum(scale_x, scale_y);

            f32 viewport_width  = CORE->window.width  * CORE->window.scale;
            f32 viewport_height = CORE->window.height * CORE->window.scale;

            CORE->window.viewport_min_x = (width  - viewport_width) / 2;
            CORE->window.viewport_min_y = (height - viewport_height) / 2;
            CORE->window.viewport_max_x = CORE->window.viewport_min_x + viewport_width;
            CORE->window.viewport_max_y = CORE->window.viewport_min_y + viewport_height;

            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms648383(v=vs.85).aspx
            // TODO: ClipCursor 

            // PUN_RUNTIME_WINDOW_RESIZE(width, height);
            return 0;
        }
        break;
    }

    return DefWindowProcA(window, message, wp, lp);
}

void
win32_update_mouse_position_()
{
    RECT client;
    GetClientRect(win32_.window, &client);

    POINT mouse_point;
    GetCursorPos(&mouse_point);
    ScreenToClient(win32_.window, &mouse_point);
    mouse_point.x = clamp(mouse_point.x, 0, client.right);
    mouse_point.y = clamp(mouse_point.y, 0, client.bottom);

    CORE->mouse_x = ((f32)mouse_point.x) / CORE->window.scale;
    CORE->mouse_y = ((f32)mouse_point.y) / CORE->window.scale;

    // TODO: GetSystemMetrics(SM_SWAPBUTTON)?
}

//
//
//

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    win32_.fullscreen_placement.length = sizeof(WINDOWPLACEMENT);
    win32_.cursor = LoadCursor(0, IDC_ARROW);
    win32_.instance = instance;

    MMRESULT mmres = timeBeginPeriod(1);
    if (mmres == TIMERR_NOERROR || mmres == TIMERR_NOCANDO ) {
        // Cannot set 1MS Sleep() resolution.
    }

#ifdef PUNITY_OPENGL
    win32_gl_init_();
#endif


    punity_init(command_line);

    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_callback_;
    window_class.hInstance = win32_.instance;
    // window_class.hIcon = (HICON)LoadImage(0, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    window_class.hIcon = (HICON)LoadIcon(instance, "icon.ico");
    // Default cursor.
    // window_class.hCursor = LoadCursor(0, IDC_ARROW);
    // Hidden cursor.
    window_class.hCursor = NULL;
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = "Punity";

    if (!RegisterClassA(&window_class)) {
        printf("RegisterClassA failed.\n");
        return 1;
    }

    // Center window
    RECT rc;
    int screen_width  = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    int window_width  = CORE->window.width  * CORE->window.scale;
    int window_height = CORE->window.height * CORE->window.scale;
    rc.left = (screen_width - (window_width)) / 2;
    rc.top = (screen_height - (window_height)) / 2;
    rc.right = rc.left + window_width;
    rc.bottom = rc.top + window_height;
    ASSERT(AdjustWindowRect(&rc, WIN32_DEFAULT_STYLE_, FALSE) != 0);

    // Create window.
    win32_.window = CreateWindowExA(
            0,
            window_class.lpszClassName,
            *win32_.window_init_title ? win32_.window_init_title : "Punity",
            WIN32_DEFAULT_STYLE_,
            rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top,
            0, 0,
            win32_.instance,
            0);

    if (!win32_.window) {
        printf("CreateWindowExA failed.\n");
        return 1;
    }

    if (win32_.window_init_flags) {
        window_fullscreen_toggle();
    }

#ifdef PUNITY_OPENGL
    win32_gl_window_init_(win32_.window);

    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &win32_.gl_texture);
    glBindTexture(GL_TEXTURE_2D, win32_.gl_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
    BITMAPINFO window_bmi = (BITMAPINFO){0};
    memset(&window_bmi, 0, sizeof(BITMAPINFO));
    window_bmi.bmiHeader.biSize = sizeof(window_bmi.bmiHeader);
    window_bmi.bmiHeader.biWidth = CORE->window.width;
    window_bmi.bmiHeader.biHeight = CORE->window.height;
    window_bmi.bmiHeader.biPlanes = 1;
    window_bmi.bmiHeader.biBitCount = 32;
    window_bmi.bmiHeader.biCompression = BI_RGB;
#endif
    
    u32 *window_buffer = 0;
    window_buffer = bank_push(CORE->stack, (CORE->window.width * 4) * CORE->window.height);
    ASSERT(window_buffer);

    // Sound

    if (win32_sound_init_(win32_.window) == 0) {
        win32_.audio_buffer = 0;
    }

    ShowCursor(FALSE);
    ShowWindow(win32_.window, SW_SHOW);

    win32_update_mouse_position_();

    int x, y;
    u32 *window_row;
    u8 *canvas_it;
    BankState stack_state;
    MSG message;
    while (CORE->running)
    {
        punity_frame_begin();

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) {
                CORE->running = 0;
                goto end;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        win32_update_mouse_position_();

        punity_frame_step();

        if (win32_.audio_buffer) {
            win32_sound_step_();
        }

        window_row = window_buffer;
        canvas_it = CORE->window.buffer;
        for (y = CORE->window.height; y != 0; --y) {
            window_row = window_buffer + ((y - 1) * CORE->window.width);
            for (x = 0; x != CORE->window.width; ++x) {
                *(window_row++) = CORE->palette->colors[*canvas_it++].rgba;
            }
        }

#if PUNITY_OPENGL
        glClearColor(
            CORE->background.r / 255.0f,
            CORE->background.g / 255.0f,
            CORE->background.b / 255.0f,
            1.0f);

        glClear(GL_COLOR_BUFFER_BIT);

        f32 window_scaled_width  = CORE->window.width  * CORE->window.scale;
        f32 window_scaled_height = CORE->window.height * CORE->window.scale;

        glViewport(0.f, 0.f, window_scaled_width, window_scaled_height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // glOrtho(0.0, window_scaled_width, window_scaled_height, 0.0, 1.0, -1.0);
        glOrtho(0.0, window_scaled_width, 0.0, window_scaled_height, 1.0, -1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glBindTexture(GL_TEXTURE_2D, win32_.gl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CORE->window.width, CORE->window.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, window_buffer);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(window_scaled_width, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(window_scaled_width, window_scaled_height);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, window_scaled_height);
        glEnd();

        HDC dc = GetDC(win32_.window);
        SwapBuffers(dc);
        ReleaseDC(win32_.window, dc);
#else
        HDC dc = GetDC(win32_.window);
        // TODO: Draw the background.
        StretchDIBits(dc,
                      CORE->window.viewport_min_x,
                      CORE->window.viewport_min_y,
                      CORE->window.viewport_max_x - CORE->window.viewport_min_x,
                      CORE->window.viewport_max_y - CORE->window.viewport_min_y,
                      0, 0, CORE->window.width, CORE->window.height,
                      window_buffer,
                      &window_bmi,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        ReleaseDC(win32_.window, dc);
#endif

        punity_frame_end();
    }

end:;

    return 0;
}

#endif // PUN_RUNTIME_WINDOWS
#endif // if PUNITY_LIB == 0

#endif // else PUN_RUNTIME_SDL

#endif // PUNITY_IMPLEMENTATION
