#include "core/core.h"
#include "main.h"

#ifdef ASSETS

void
assets(AssetWrite *out)
{
    assets_add_image_palette(out, "palette.png");
    assets_add_image_set(out, "font.png", 4, 7, (Rect){1, 0, 0, 0});
    assets_add_image_set(out, "icons.png", 7, 7, (Rect){0});
    assets_add_image_set(out, "walk.png", 8, 8, (Rect){0});
    assets_add_image_set(out, "tileset.png", 8, 8, (Rect){0});
    assets_add_tile_map(out, "map.csv");
}

#else

void
init()
{
    memcpy(_program->palette, &asset_palette->colors, asset_palette->colors_count * sizeof(Color));
    _program->font = asset_font;

    _state->a = 0;
}

void
debug_buttons(s32 x, s32 y)
{
    Rect r = {x, y, x + asset_icons->cw, y + asset_icons->ch};
    u8 cf, cb;
    for (u8 i = 0; i != _Button_MAX; ++i)
    {
        cf = COLOR_SHADE_5;
        cb = 0;
        if (_program->buttons[i].state & ButtonState_EndedDown) {
            cf = COLOR_WHITE;
            cb = COLOR_BLACK;
        } else if (_program->buttons[i].state & ButtonState_EndedUp) {
            cf = COLOR_WHITE;
            cb = COLOR_BLACK;
        } else if (_program->buttons[i].state & ButtonState_Down) {
            cf = COLOR_BLACK;
            cb = COLOR_SHADE_5;
        }


        if (cb) {
            draw_rect(r, cb);
        }

        if (cf) {
            draw_sprite(r.min_x, r.min_y,
                        asset_icons, i,
                        DrawSpriteMode_Mask, cf);
        }

        rect_offset(&r, asset_icons->cw, 0);
    }
}

void
debug_fps(s32 x, s32 y, r64 fps)
{
    char str[8] = "***.***";
    u32 time_step = (u32)(fps);
    if (time_step <= 999999 && time_step >= 0) {
        sprintf(str, "%.3d", time_step / 1000);
        str[3] = '.';
        sprintf(str+4, "%.3d", time_step % 1000);
    }
    draw_text(x, y, str, COLOR_BLACK, COLOR_WHITE);
}

void
step()
{
    draw_set(COLOR_BLACK);

    if ((_program->frame & 1) == 0) {
        _state->a = (_state->a + 1) % asset_walk->count;
    }

    draw_sprite(24, 16,
                asset_walk, _state->a,
                0, 0);

    u16 *tile = asset_data(asset_map);
    for (memi y = 0; y < 16; y++) {
        for (memi x = 0; x < 16; x++) {
            if (tile[x] != 0xFFFF) {
                draw_sprite(x * asset_tileset->cw,
                            y * asset_tileset->ch,
                            asset_tileset,
                            tile[x],
                            0, 0);
            }
        }
        tile += asset_map->w;
    }

    debug_buttons(4, SCREEN_HEIGHT - asset_icons->ch - 4);
    debug_fps(SCREEN_WIDTH - (7 * asset_font->cw) - 4,
              SCREEN_HEIGHT - asset_font->ch - 4,
              _program->time_step);
}

#endif
