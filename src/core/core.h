#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <stdint.h>
#include <stddef.h>
#include "config.h"

#define internal static

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef float    r32;
typedef double   r64;
typedef size_t   memi;

#define is_number(c) ((c) >= '0' && (c) <= '9')
#define is_separator_post(c) ((c) == ' ' || (c) <= '.' || (c) <= ',' || (c) <= ';')
#define is_separator_pre(c)  ((c) == '(' || (c) <= '[' || (c) <= '{' || (c) <= '<' || (c) == 0)

typedef struct Color
{
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
} Color;

typedef struct Rect
{
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;
} Rect;

//
// Type helpers.
//

Color
color_make(u8 r, u8 g, u8 b, u8 a)
{
    Color c = { r, g, b, a };
    return c;
}

Rect
rect_make(s32 min_x, s32 min_y, s32 max_x, s32 max_y)
{
    Rect r = { min_x, min_y, max_x, max_y };
    return r;
}

Rect
rect_make_wh(s32 min_x, s32 min_y, s32 w, s32 h)
{
    Rect r = { min_x, min_y, min_x + w, min_y + h };
    return r;
}

void
rect_offset(Rect *r, s32 dx, s32 dy)
{
    r->min_x += dx;
    r->max_x += dx;
    r->min_y += dy;
    r->max_y += dy;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SWAP(T, a, b) do { T tmp__ = a; a = b; b = tmp__; } while (0)

//
// Assets
//

typedef enum
{
    Asset_Raw,
    Asset_C,
    Asset_Image,
    Asset_SpriteList,
    Asset_Palette,
    Asset_TileMap
}
AssetType;

//typedef struct AssetDescriptor {
//    char *name;
//    AssetType type;
//    u32 flags;
//} AssetDescriptor;

typedef struct Asset
{
    u8  type;
    u32 size;
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
}
Image;

typedef struct ImageSet
{
    Asset asset;
    u8 cw;
    u8 ch;
    u16 count;
    Rect padding;
}
ImageSet;

typedef struct TileMap
{
    Asset asset;
    u32 w;
    u32 h;
}
TileMap;

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

typedef enum Buttons
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
Buttons;

typedef struct Button
{
    s32 keys[4];
    u8 state;
}
Button;

#ifdef ASSETS
//
// Assets
//

typedef struct AssetWrite {
    FILE *header;
    FILE *binary;
    memi binary_offset;
    Palette palette;
}
AssetWrite;

typedef enum AssetWriteFlags
{
    AssetWriteFlag_ImageMatchPalette = 0x01,
}
AssetWriteFlags;

extern void assets(AssetWrite *out);

void assets_add_image(AssetWrite *out, char *path, u32 flags);
void assets_add_image_palette(AssetWrite *out, char *path);
void assets_add_image_set(AssetWrite *out, char *path, u32 cw, u32 ch, Rect padding);
void assets_add_tile_map(AssetWrite *out, char *name);
#endif

//
// Program
//

#ifndef ASSETS
#include "src/res.h"
#endif

#include "src/main.h"


typedef struct Program
{
    u8 _screen[SCREEN_WIDTH * SCREEN_HEIGHT];
//#ifdef ASSETS_SIZE
//    u8 _assets[ASSETS_SIZE];
//#endif

    // True when the application is running.
    u8 running;
    // Current frame.
    u32 frame;
    // State of the input buttons.
    Button buttons[_Button_MAX];
    // Palette used to render screen.
    Color palette[256];

    // Time taken to execute step().
    r64 time_step;
    // Time taken to execute whole frame.
    r64 time_frame;

    // Screen bitmap;
    Bitmap screen;
    // Screen coordinates.
    Rect screen_rect;

    // Current drawing font.
    ImageSet *font;
    // Current target bitmap to draw into.
    Bitmap *bitmap;
    Rect bitmap_rect;
    // Current clip rectangle.

    // Current color to use for drawing.
    // Only low 8-bits are actually used.
    // Set to 0x100 to ignore.
    u16 color;

    // Custom state;
    ProgramState state;

} Program;

static Program *_program;
static ProgramState *_state;

#define asset_data(asset) ((void*)(asset + 1))

extern void init();
extern void step();

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
// Graphics
//

void draw_set(u8 color);
void draw_clip(Rect draw_clip);

// Draws sprite.
typedef enum
{
    // Color is used instead of sprite non-transparent pixels.
    DrawSpriteMode_Mask   = 0x02,
    // Frame is drawn around the sprite.
    // DrawSpriteMode_Frame  = 0x04,
}
DrawSpriteMode;

void draw_sprite(s32 x, s32 y, ImageSet *sprites, u16 index, u8 mode, u8 mask);
void draw_sprite_ex(s32 x, s32 y, ImageSet *sprites, u16 index, u8 mode, u8 mask, u8 frame);

void draw_rect(Rect rect, u8 color);
void draw_rect_i(s32 x, s32 y, s32 w, s32 h, u8 color);

typedef struct DrawTextResult
{
    s32 x;
    s32 y;
}
DrawTextResult;

DrawTextResult draw_text(s32 x, s32 y, char *text, u8 cf, u8 cb);

//
// Recorder
//

#ifdef RECORDER
void rec_begin(char *path);
s32  rec_active();
void rec_end(void);
#endif

#endif // CORE_H
