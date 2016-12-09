// Build with: build example-tiled

#define PUNITY_IMPLEMENTATION
#include "punity.h"

#define TILED_IMPLEMENTATION
#include "punity-tiled.h"


typedef struct Game_
{
    Bitmap font;
    Bitmap tileset;
    TiledScene tiled;
}
Game;

static Game *GAME = 0;

TILED_CALLBACK(tiled_callback)
{
    for (int i = 0; i != properties_count; ++i) {
        switch (properties[i].value->type)
        {
        case json_string:
            LOG("%s = %s\n", properties[i].name, properties[i].value->string.ptr);
            break;
        }
    }
}

int
init()
{
    CORE->window.width = 16 * 8;
    CORE->window.height = 16 * 8;
    CORE->window.scale = 5;

    GAME = bank_push_t(CORE->storage, Game, 1);
    font_load_resource(&GAME->font, "font.png", 4, 7);
    CORE->canvas.font = &GAME->font;

    f64 p = perf_get();
    Tiled tiled;
    tiled_init(&tiled);
    tiledscene_load_resource(&tiled, &GAME->tiled, "map_test.json", tiled_callback, 0);
    tiled_free(&tiled);
    LOG("tiledscene_load_resource %fms\n", (perf_get() - p) * 1e3);

    TiledItem *item = tiledscene_find(&GAME->tiled, "Tile Layer 1", 0);
    ASSERT(item);
    ASSERT(item->type == TiledType_TileMap);

    return 1;
}

void
step()
{
    if (key_pressed(KEY_F10)) {
        window_fullscreen_toggle();
    }

    canvas_clear(1);

    for (TiledItem *item = GAME->tiled.items;
         item != GAME->tiled.items_end;
         item = tileditem_next(item))
    {
        switch (item->type)
        {
        case TiledType_TileMap:
            tilemap_draw(&item->tilemap);
            break;
        case TiledType_Rectangle:
            rect_draw(item->rect, 2);
            break;
        case TiledType_Image:
            bitmap_draw(item->tile.tileset, item->rect.min_x, item->rect.min_y, 0, 0, 0);
            break;
        }
    }
   
    char buf[256];
    sprintf(buf, "%.3fms %.3f", CORE->perf_step * 1e3, CORE->time_delta);
    text_draw(buf, 0, 0, 2);
}