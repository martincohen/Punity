#ifndef DEBUG_H
#define DEBUG_H

#include "main.h"

void
debug_buttons(i32 x, i32 y)
{
    V4i r = {{x, y, x + res_icons->cw, y + res_icons->ch}};
    u8 cf, cb;
    for (u8 i = 0; i != _Button_MAX; ++i)
    {
        cf = COLOR_SHADE_5;
        cb = 0;
        if (PROGRAM->buttons[i].state & ButtonState_EndedDown) {
            cf = COLOR_WHITE;
            cb = COLOR_BLACK;
        } else if (PROGRAM->buttons[i].state & ButtonState_EndedUp) {
            cf = COLOR_WHITE;
            cb = COLOR_BLACK;
        } else if (PROGRAM->buttons[i].state & ButtonState_Down) {
            cf = COLOR_BLACK;
            cb = COLOR_SHADE_5;
        }


        if (cb) {
            rect_draw(r, cb);
        }

        if (cf) {
            image_set_draw(r.min_x, r.min_y,
                     res_icons, i,
                     DrawSpriteMode_Mask, cf);
        }

        rect_tr(&r, res_icons->cw, 0);
    }
}

void
debug_fps(i32 x, i32 y, f64 fps)
{
    char str[8] = "***.***";
    u32 time_step = (u32)(fps);
    if (time_step <= 999999) {
        sprintf(str, "%.3d", time_step / 1000);
        str[3] = '.';
        sprintf(str+4, "%.3d", time_step % 1000);
    }
    text_draw(x, y, str, COLOR_BLACK, COLOR_WHITE);
}

void
debug_world_colliders_count(World *W)
{
    char buf[4];
    i32 y, x;
    WorldCell *cell;
    for (y = W->cell_rect.min_y; y != W->cell_rect.max_y; ++y) {
        for (x = W->cell_rect.min_x; x != W->cell_rect.max_x; ++x) {
            cell = world_cell(W, x, y);
            rect_draw(v4i_make_size(x * WORLD_CELL_SIZE, y * WORLD_CELL_SIZE, 1, 1), COLOR_ACCENT);
            if (cell && cell->colliders_count) {
                sprintf(buf, "%d", cell->colliders_count);
                text_draw(x * WORLD_CELL_SIZE + 2, y * WORLD_CELL_SIZE + 2, buf, COLOR_WHITE, COLOR_BLACK);
            }
            cell++;
        }
    }
}

void
debug_world_bucket_stats(World *W, i32 x, i32 y)
{
    static char str[1024];
    sprintf(str, "Cl:%d Co:%d", W->cells_count, W->colliders_count);
    text_draw(x, y, str, COLOR_BLACK, COLOR_WHITE);
}

void
debug_world_bucket_cells(World *W, i32 x, i32 y)
{
    u32 c;
    u32 _x, _y;
    rect_draw(v4i_make_size(x-1, y-1, 18, 18), 2);
    for (_y = 0; _y != 16; _y++) {
        for (_x = 0; _x != 16; _x++) {
            c = W->buckets_cells_count[_x + _y * 16];
            pixel_draw(x + _x, y + _y, c ? (2 + MIN((i32)c, 6)) : 0);
        }
    }
}

#endif // DEBUG_H

