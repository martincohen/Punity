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
clip_rect(V4i *R, V4i *C)
{
    R->min_x = MIN(C->max_x, MAX(R->min_x, C->min_x));
    R->min_y = MIN(C->max_y, MAX(R->min_y, C->min_y));
    R->max_x = MAX(C->min_x, MIN(R->max_x, C->max_x));
    R->max_y = MAX(C->min_y, MIN(R->max_y, C->max_y));
}

// Returns 0 if R does not intersect with C.
// Returns 1 if R has NOT been clipped (is fully visible).
// Returns 2 if R has been clipped.
i32
clip_rect_with_offsets(V4i *R, V4i *C, i32 *ox, i32 *oy)
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

#if 0
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
#endif

//
// Graphics
//

void
draw_set(u8 color)
{
    memset(PROGRAM->bitmap->data, color, bitmap_size(PROGRAM->bitmap));
}

#define _BLIT(color, source_increment) \
    for (_y = 0; _y != h; ++_y) { \
        for (_x = 0; _x != w; ++_x) { \
            dst[_x] = *src ? color : dst[_x]; \
            source_increment; \
        } \
        dst += dst_fill; \
        src += src_fill; \
    }

//internal void
//_draw_sprite_frame(Rect *r, s32 ox, s32 oy, SpriteList *sprites, u8 index, u8 color)
//{

//}

void
set_draw(i32 x, i32 y, ImageSet *set, u16 index, u8 mode, u8 mask)
{
    assert(index < set->count);

    V4i r = { x - set->pivot_x,
              y - set->pivot_y,
              x - set->pivot_x + set->cw,
              y - set->pivot_y + set->ch
            };

    rect_tr(&r, PROGRAM->tx, PROGRAM->ty);
    // clip_rect(&r, &program->bitmap_rect);
    i32 ox = 0;
    i32 oy = 0;
    if (clip_rect_with_offsets(&r, &PROGRAM->bitmap_rect, &ox, &oy))
    {
        i32 w = r.max_x - r.min_x;
        i32 h = r.max_y - r.min_y;
        i32 sw = set->cw;
        i32 sh = set->ch;

        u32 dst_fill = PROGRAM->bitmap_rect.max_x - PROGRAM->bitmap_rect.min_x;
        u8 *dst = PROGRAM->bitmap->data + r.min_x + (r.min_y * dst_fill);

        i32 _x, _y;
        if ((mode & DrawSpriteMode_FlipH) == 0)
        {
            u32 src_fill = sw - w;
            u8 *src = asset_data(set)
                     + (index * (sw * sh))
                     + (ox + (oy * sw));
            if (mode & DrawSpriteMode_Mask) {
                _BLIT(mask, src++);
            } else {
                _BLIT(*src, src++);
            }
        }
        else
        {
            u32 src_fill = sw + w;
            u8 *src = asset_data(set)
                     + (index * (sw * sh))
                     + (ox + (oy * sw))
                     + (w - 1);

            if (mode & DrawSpriteMode_Mask) {
                _BLIT(mask, src--);
            } else {
                _BLIT(*src, src--);
            }
        }
    }
}

#undef _BLIT

//   +
// + o +
//   +

void
set_draw_ex(i32 x, i32 y, ImageSet *sprites, u16 index, u8 mode, u8 mask, u8 frame)
{
    set_draw(x - 1, y, sprites, index, DrawSpriteMode_Mask, frame);
    set_draw(x + 1, y, sprites, index, DrawSpriteMode_Mask, frame);
    set_draw(x, y - 1, sprites, index, DrawSpriteMode_Mask, frame);
    set_draw(x, y + 1, sprites, index, DrawSpriteMode_Mask, frame);
    set_draw(x, y, sprites, index, mode, mask);
}

void
rect_draw(V4i r, u8 color)
{
    rect_tr(&r, PROGRAM->tx, PROGRAM->ty);
    clip_rect(&r, &PROGRAM->bitmap_rect);
    if (r.max_x != r.min_x && r.max_y != r.min_y)
    {
        i32 row_pitch = PROGRAM->bitmap_rect.max_x - PROGRAM->bitmap_rect.min_x;
        u8 *row = PROGRAM->bitmap->data + r.min_x + (r.min_y * row_pitch);
        i32 w = r.max_x - r.min_x;
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
text_draw(i32 x, i32 y, char *text, u8 cf, u8 cb)
{
    assert(PROGRAM->font);

    V4i r = recti_make_size(x, y, PROGRAM->font->cw, PROGRAM->font->ch);

    if ((x + PROGRAM->tx) < PROGRAM->bitmap_rect.max_x &&
        (y + PROGRAM->ty) < PROGRAM->bitmap_rect.max_y)
    {
        // rect_tr(&r, PROGRAM->tx, PROGRAM->ty);

        V4i rect;
        ImageSet *font = PROGRAM->font;
        char *anchor = text;
        for (;;)
        {
            text = find_next(anchor, '\n');

            if (cb) {
                rect.min_x = x - font->padding.min_x;
                rect.min_y = y - font->padding.min_y;
                rect.max_x = x + (text - anchor) * font->cw + font->padding.max_x;
                rect.max_y = y + font->ch + font->padding.max_y;
                rect_draw(rect, cb);
            }
            i32 xp = x;
            while (anchor != text) {
                if (*anchor != ' ') {
                    set_draw(xp,
                                y,
                                PROGRAM->font,
                                *anchor,
                                DrawSpriteMode_Mask,
                                cf);
                }
                anchor++;
                xp += PROGRAM->font->cw;
            }
            if (*text == 0) {
                break;
            }
            text++;
            anchor = text;
            rect_tr(&r, 0, PROGRAM->font->ch);
        }
    }

    DrawTextResult res = { r.max_x, r.min_y };
    return res;
}

//
//
//
