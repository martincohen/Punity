#ifndef DEBUG_H
#define DEBUG_H

#include "core/core.h"

void
debug_buttons(i32 x, i32 y)
{
    V4i r = {x, y, x + asset_icons->cw, y + asset_icons->ch};
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
            set_draw(r.min_x, r.min_y,
                     asset_icons, i,
                     DrawSpriteMode_Mask, cf);
        }

        rect_tr(&r, asset_icons->cw, 0);
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
    u32 y, x;
    WorldCell *cell;
    for (y = W->cell_rect.min_y; y != W->cell_rect.max_y; ++y) {
        for (x = W->cell_rect.min_x; x != W->cell_rect.max_x; ++x) {
            cell = world_cell(W, x, y);
            rect_draw(recti_make_size(x * WORLD_CELL_SIZE, y * WORLD_CELL_SIZE, 1, 1), COLOR_ACCENT);
            if (cell && cell->colliders_count) {
                sprintf(buf, "%d", cell->colliders_count);
                text_draw(x * WORLD_CELL_SIZE + 2, y * WORLD_CELL_SIZE + 2, buf, COLOR_WHITE, COLOR_BLACK);
            }
            cell++;
        }
    }
}

#endif // DEBUG_H

