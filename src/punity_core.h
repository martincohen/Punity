#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "config.h"

#define internal static
#define unused(x) (void)x

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
typedef size_t   memi;
typedef i32      boolean;

#define is_number(c) ((c) >= '0' && (c) <= '9')
#define is_separator_post(c) ((c) == ' ' || (c) <= '.' || (c) <= ',' || (c) <= ';')
#define is_separator_pre(c)  ((c) == '(' || (c) <= '[' || (c) <= '{' || (c) <= '<' || (c) == 0)

#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)

#define MAX(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; \
  })

#define MIN(a, b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; \
  })

#define SWAP(T, x, y) do { T __tmp = x; x = y; y = __tmp; } while (0)

#define ASSERT assert
#define ASSERT_DEBUG assert

#define ceil_div(n, a) \
    (((n) + (a-1))/(a))

enum DirectionFlags
{
    DirectionFlag_Left   = 0x01,
    DirectionFlag_Top    = 0x02,
    DirectionFlag_Right  = 0x04,
    DirectionFlag_Bottom = 0x08,
};

inline char *strpush(char *dest, char *src);
inline char *strdup(const char *str);
//
// Vectors
//

#define v2_inner(ax, ay, bx, by) \
    ((ax)*(bx) + (ay)*(by))

//
// Rectangles
//

typedef union V2i
{
    struct {
        i32 x;
        i32 y;
    };
    struct {
        i32 w;
        i32 h;
    };
}
V2i;

typedef union V2f
{
    struct {
        i32 x;
        i32 y;
    };
    struct {
        i32 w;
        i32 h;
    };
}
V2f;

typedef union V4i
{
    struct {
        i32 min_x;
        i32 min_y;
        i32 max_x;
        i32 max_y;
    };
    struct {
        i32 x;
        i32 y;
        i32 w;
        i32 h;
    };
}
V4i;

typedef union V4f
{
    struct {
        f32 min_x;
        f32 min_y;
        f32 max_x;
        f32 max_y;
    };
    struct {
        f32 x;
        f32 y;
        f32 w;
        f32 h;
    };
}
V4f;

//RectI
//recti_make(s32 min_x, s32 min_y, s32 max_x, s32 max_y)
//{
//    RectI r = { min_x, min_y, max_x, max_y };
//    return r;
//}

inline V4i v4i_make_size(i32 x, i32 y, i32 w, i32 h);
inline V4f v4f_make_size(f32 x, f32 y, f32 w, f32 h);

#define rect_tr(r, tx, ty) \
    (r)->min_x += tx; \
    (r)->min_y += ty; \
    (r)->max_x += tx; \
    (r)->max_y += ty;

inline V4i
recti_cell(V4i *r, i32 cw, i32 ch)
{
    V4i result;
    result.min_x = ((i32)r->min_x / cw);
    result.min_y = ((i32)r->min_y / ch);
    result.max_x = ceil_div((i32)r->max_x, cw);
    result.max_y = ceil_div((i32)r->max_y, ch);

    return result;
}

inline V4i
rectf_to_cell(V4f *r, i32 cw, i32 ch)
{
    V4i result;
    result.min_x = ((i32)r->min_x / cw);
    result.min_y = ((i32)r->min_y / ch);
    result.max_x = ceil_div((i32)r->max_x, cw);
    result.max_y = ceil_div((i32)r->max_y, ch);

    return result;
}

//
// Color
//


typedef union Color
{
    struct
    {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };
    u32 rgba;
}
Color;

inline Color
color_make(u8 r, u8 g, u8 b, u8 a)
{
    Color c = { {r, g, b, a} };
    return c;
}

//
// Assets
//

typedef enum
{
    AssetType_Raw,
    AssetType_C,
    AssetType_Image,
    AssetType_ImageSet,
    AssetType_Palette,
    AssetType_TileMap,

    AssetType_Custom
}
AssetType;

//typedef struct AssetDescriptor {
//    char *name;
//    AssetType type;
//    u32 flags;
//} AssetDescriptor;


// Deprecated.
typedef struct Asset
{
    u8  type;
    u32 size;
    // u8  data[];
}
Asset;

//
// Asset Types
//

typedef struct Palette
{
    Asset asset;
    Color colors[256];
    u8 colors_count;
}
Palette;

typedef struct Image
{
    Asset asset;
    u32 w;
    u32 h;
    // u8 data[];
}
Image;

typedef struct ImageSet
{
    Asset asset;
    u8 cw;
    u8 ch;
    i8 pivot_x;
    i8 pivot_y;
    u16 count;
    V4i padding;
    // u8 data[];
}
ImageSet;

typedef struct TileMap
{
    Asset asset;
    u32 w;
    u32 h;
    // u16 data[];
}
TileMap;

#define image_data(Asset) ((u8*)(Asset + 1))
#define image_size(W, H)  (sizeof(Image) + (W) * (H) * sizeof(u8))
#define image_set_data(Asset) ((u8*)(Asset + 1))
#define tile_map_data(Asset) ((u16*)(Asset + 1))
#define tile_map_size(W, H)  (sizeof(TileMap) + (W) * (H) * sizeof(u16))

//
//
//

// TODO: Rename to surface or framebuffer.

typedef struct Bitmap
{
    u8 *data;
    u32 w;
    u32 h;
}
Bitmap;

#define bitmap_size(bitmap) (bitmap->w * bitmap->h)

//
//
//

typedef enum ButtonState
{
    ButtonState_Up           = 0x00,
    ButtonState_Down         = 0x01,
    ButtonState_EndedUp      = 0x02,
    ButtonState_EndedDown    = 0x04,
}
ButtonState;

typedef enum ButtonId
{
    Button_Left = 0,
    Button_Right,
    Button_Up,
    Button_Down,
    Button_A,
    Button_B,
    Button_X,
    Button_Y,
    _Button_MAX
}
ButtonId;

typedef struct Button
{
    i32 keys[4];
    u8 state;
}
Button;

//
// Program
//

// #define asset_data(asset) ((void*)(asset + 1))

void init();
void step();

//
// IO
//

typedef struct IOReadResult
{
    u32 size;
    u8 *data;
}
IOReadResult;

IOReadResult io_read(char *path);

//
// Input
//

#define button_down(button) \
    (PROGRAM->buttons[button].state & ButtonState_Down)

#define button_ended_down(button) \
    (PROGRAM->buttons[button].state & ButtonState_EndedDown)

#define button_up(button) \
    (PROGRAM->buttons[button].state == 0)

#define button_ended_up(button) \
    (PROGRAM->buttons[button].state & ButtonState_EndedUp)

//
// Graphics
//

void draw_set(u8 color);
void draw_clip(V4i draw_clip);

void pixel_draw(i32 x, i32 y, u8 color);

// Draws sprite.
typedef enum
{
    // Color is used instead of sprite non-transparent pixels.
    DrawSpriteMode_Mask   = 0x02,
    DrawSpriteMode_FlipV  = 0x04,
    DrawSpriteMode_FlipH  = 0x08,
    // Frame is drawn around the sprite.
    // DrawSpriteMode_Frame  = 0x04,
}
DrawSpriteMode;

void image_set_draw(i32 x, i32 y, ImageSet *set, u16 index, u8 mode, u8 mask);
void image_set_draw_ex(i32 x, i32 y, ImageSet *set, u16 index, u8 mode, u8 mask, u8 frame);

void rect_draw(V4i rect, u8 color);

typedef struct DrawTextResult
{
    i32 x;
    i32 y;
}
DrawTextResult;

DrawTextResult text_draw(i32 x, i32 y, char *text, u8 cf, u8 cb);

//
// Recorder
//

#ifdef RECORDER
void rec_begin(char *path);
i32  rec_active();
void rec_end(void);
#endif

//
// Program
//


struct ProgramState;

typedef struct Program
{
    // Screen buffer.
    u8 _screen[SCREEN_WIDTH * SCREEN_HEIGHT];

    // True when the application is running.
    u8 running;
    // Current frame.
    u32 frame;
    // State of the input buttons.
    Button buttons[_Button_MAX];
    // Palette used to render screen.
    Color palette[256];

    // Time taken to execute step().
    f64 time_step;
    // Time taken to execute whole frame.
    f64 time_frame;

    // Screen bitmap;
    Bitmap screen;
    // Screen coordinates.
    V4i screen_rect;

    // Current drawing font.
    ImageSet *font;
    // Current target bitmap to draw into.
    Bitmap *bitmap;
    V4i bitmap_rect;
    // Current clip rectangle.

    // Current color to use for drawing.
    // Only low 8-bits are actually used.
    // Set to 0x100 to ignore.
    u16 color;

    struct ProgramState *state;

    // Translation.
    i32 tx;
    i32 ty;

} Program;

extern Program *PROGRAM;

#endif // CORE_H
