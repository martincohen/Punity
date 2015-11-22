#include "core/core.h"

//
// IO
//

IOReadResult
io_read(char *path)
{
    IOReadResult res = {0};
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0L, SEEK_END);
        res.size = ftell(f);
        fseek(f, 0L, SEEK_SET);

        res.data = malloc(res.size + 1);
        memi s = fread(res.data, 1, res.size, f);
        fclose(f);
        res.data[res.size] = 0;

        assert(s == res.size);
    }
    return res;
}

//
// Graphics
//

static void
clip_rect(Rect *R, Rect *C)
{
    R->min_x = MIN(C->max_x, MAX(R->min_x, C->min_x));
    R->min_y = MIN(C->max_y, MAX(R->min_y, C->min_y));
    R->max_x = MAX(C->min_x, MIN(R->max_x, C->max_x));
    R->max_y = MAX(C->min_y, MIN(R->max_y, C->max_y));
}

// Returns 0 if R does not intersect with C.
// Returns 1 if R has NOT been clipped (is fully visible).
// Returns 2 if R has been clipped.
s32
clip_rect_with_offsets(Rect *R, Rect *C, s32 *ox, s32 *oy)
{
    if (R->max_x < C->min_x || R->min_x >= C->max_x ||
        R->max_y < C->min_y || R->min_y >= C->max_y) {
        return 0;
    }

    s32 res = 1;

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

s32
parse_number(char **text, u32 radix, u32 *n)
{
    if (is_number(**text)) {
        *n = 0;
        do {
            *n = (*n * radix) + (**text) - '0';
            *text++;
        } while (is_number(**text));

        return 1;
    }
    return 0;
}

//
// Graphics
//

void
draw_set(u8 color)
{
    memset(_program->bitmap->data, color, bitmap_size(_program->bitmap));
}

#define _BLIT(color) \
    s32 _x, _y; \
    for (_y = 0; _y != h; ++_y) { \
        for (_x = 0; _x != w; ++_x) { \
            dst[_x] = *src ? color : dst[_x]; \
            src++; \
        } \
        dst += dst_fill; \
        src += src_fill; \
    }

//internal void
//_draw_sprite_frame(Rect *r, s32 ox, s32 oy, SpriteList *sprites, u8 index, u8 color)
//{

//}

void
draw_sprite(s32 x, s32 y, ImageSet *set, u16 index, u8 mode, u8 mask)
{
    assert(index < set->count);

    Rect r = { x, y, x + set->cw, y + set->ch };
    // clip_rect(&r, &program->bitmap_rect);
    s32 ox = 0;
    s32 oy = 0;
    if (clip_rect_with_offsets(&r, &_program->bitmap_rect, &ox, &oy))
    {
        u32 w = r.max_x - r.min_x;
        u32 h = r.max_y - r.min_y;
        u32 sw = set->cw;
        u32 sh = set->ch;

        u32 dst_fill = _program->bitmap_rect.max_x - _program->bitmap_rect.min_x;
        u8 *_dst = _program->bitmap->data + r.min_x + (r.min_y * dst_fill);

        u32 src_fill = sw - w;
        u8 *_src = asset_data(set)
                 + (index * (sw * sh))
                 + (ox + (oy * sw));

        u8 *dst = _dst;
        u8 *src = _src;
        if (mode & DrawSpriteMode_Mask) {
            _BLIT(mask);
        } else {
            _BLIT(*src);
        }
    }
}

#undef _BLIT

//   +
// + o +
//   +

void
draw_sprite_ex(s32 x, s32 y, ImageSet *sprites, u16 index, u8 mode, u8 mask, u8 frame)
{
    draw_sprite(x - 1, y, sprites, index, DrawSpriteMode_Mask, frame);
    draw_sprite(x + 1, y, sprites, index, DrawSpriteMode_Mask, frame);
    draw_sprite(x, y - 1, sprites, index, DrawSpriteMode_Mask, frame);
    draw_sprite(x, y + 1, sprites, index, DrawSpriteMode_Mask, frame);
    draw_sprite(x, y, sprites, index, mode, mask);
}

void
draw_rect(Rect r, u8 color)
{
    clip_rect(&r, &_program->bitmap_rect);
    if (r.max_x != r.min_x && r.max_y != r.min_y)
    {
        s32 row_pitch = _program->bitmap_rect.max_x - _program->bitmap_rect.min_x;
        u8 *row = _program->bitmap->data + r.min_x + (r.min_y * row_pitch);
        s32 w = r.max_x - r.min_x;
        while (r.max_y != r.min_y)
        {
            memset(row, color, w);
            row += row_pitch;
            r.max_y--;
        }
    }
}

internal char *
find_next(char *it, char c) {
    while (*it && *it != c) it++;
    return it;
}

DrawTextResult
draw_text(s32 x, s32 y, char *text, u8 cf, u8 cb)
{
    assert(_program->font);

    Rect r = rect_make_wh(x, y, _program->font->cw, _program->font->ch);

    if (x < _program->bitmap_rect.max_x &&
        y < _program->bitmap_rect.max_y)
    {
        Rect rect;
        ImageSet *font = _program->font;
        char *anchor = text;
        for (;;)
        {
            text = find_next(anchor, '\n');

            if (cb) {
                rect.min_x = x - font->padding.min_x;
                rect.min_y = y - font->padding.min_y;
                rect.max_x = x + (text - anchor) * font->cw + font->padding.max_x;
                rect.max_y = y + font->ch + font->padding.max_y;
                draw_rect(rect, cb);
            }
            s32 xp = x;
            while (anchor != text) {
                if (*anchor != ' ') {
                    draw_sprite(xp,
                                y,
                                _program->font,
                                *anchor,
                                DrawSpriteMode_Mask,
                                cf);
                }
                anchor++;
                xp += _program->font->cw;
            }
            if (*text == 0) {
                break;
            }
            text++;
            anchor = text;
            rect_offset(&r, 0, _program->font->ch);
        }
    }

    DrawTextResult res = { r.max_x, r.min_y };
    return res;
}
