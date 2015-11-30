#include "main.h"
#include "punity_assets.h"
#include "punity_tiled.c"

// TODO: If main() gets argument "update-assets" it should call assets().
// TODO: Encode the target storage paths better.

// IDs corresponding to Tiled's tile ID.
#define META_TILE_SOLID (0)
#define META_TILE_PLAYER (16)

internal TileMap *
_map_new(u32 w, u32 h)
{
    TileMap *map = calloc(tile_map_size(w, h), 1);
    map->asset.size = tile_map_size(w, h);
    map->asset.type = AssetType_TileMap;
    map->w = w;
    map->h = h;
    return map;
}

internal i32
_level_match_cell(TiledTileLayer *layer, i32 x, i32 y, TiledTileSet *tileset)
{
    if (x < 0 || x >= layer->w ||
        y < 0 || y >= layer->h) {
        return -1;
    }
    TiledCell *cell = &layer->cells[x + y * layer->w];
    return cell->tileset == tileset ? (i32)(cell->tile) : -1;
}

internal void
_level_read_meta(Level *level, TiledTileMap *map, TiledTileLayer *read_layer, TiledTileSet *meta)
{
    ASSERT((u32)read_layer->w == map->w && (u32)read_layer->h == map->h);
    level->map_collision = _map_new(map->w, map->h);
    level->map_meta = _map_new(map->w, map->h);

    TiledCell *cell;
    u16 c;
    u32 x, y;
    for (y = 0; y != map->h; ++y) {
        for (x = 0; x != map->w; ++x) {
            cell = read_layer->cells + (x + y * map->w);
            if (cell->gid)
            {
                ASSERT(cell->tileset == meta);

                switch (cell->tile)
                {
                case META_TILE_SOLID:
                    c = 0;

                    c |= (_level_match_cell(read_layer, x-1, y, meta) != META_TILE_SOLID) * DirectionFlag_Left;
                    c |= (_level_match_cell(read_layer, x, y-1, meta) != META_TILE_SOLID) * DirectionFlag_Top;
                    c |= (_level_match_cell(read_layer, x+1, y, meta) != META_TILE_SOLID) * DirectionFlag_Right;
                    c |= (_level_match_cell(read_layer, x, y+1, meta) != META_TILE_SOLID) * DirectionFlag_Bottom;

                    if (c) {
                        tile_map_data(level->map_collision)[x + y * map->w] = c;
                    }
                    break;
                case META_TILE_PLAYER:
                    level->player_start.x = x;
                    level->player_start.y = y;
                    break;
                }
            }
        }
    }
}

internal void
_level_read_layer(Level *level, TiledTileMap *map, TiledTileLayer *read_layer, TiledTileSet *tileset)
{
    ASSERT(read_layer->w == (i32)map->w && read_layer->h == (i32)map->h);
    TileMap *layer = level->layers[level->layers_count++] = _map_new(map->w, map->h);
    layer->w = map->w;
    layer->h = map->h;
    TiledCell *cell;
    i32 x, y;
    for (y = 0; y != read_layer->h; ++y) {
        for (x = 0; x != read_layer->w; ++x) {
            cell = read_layer->cells + x + y * read_layer->w;
            if (cell->tile && cell->tileset == tileset) {
                ASSERT(cell->tile < 0x10000);
                ASSERT(cell->tile < (tileset->tiles_count));
                tile_map_data(layer)[x + y * layer->w] = (u16)cell->tile;
            }
        }
    }
}

internal Level *
_level_read(char *path)
{
    Level *level = calloc(sizeof(Level), 1);
    TiledTileMap *map = tiled_read(path);

    u32 i;

    TiledTileSet *tileset_main = 0;
    TiledTileSet *tileset_meta = 0;
    TiledTileSet *tileset;
    for (i = 0; i != map->tilesets_count; ++i) {
        tileset = map->tilesets[i];
        if (!tileset_main && strcmp(tileset->name, "main") == 0) {
            tileset_main = tileset;
        } else if (!tileset_meta && strcmp(tileset->name, "meta") == 0) {
            tileset_meta = tileset;
        }
    }

    ASSERT(tileset_main);
    ASSERT(tileset_meta);

    TiledTileLayer *layer;
    for (i = 0; i != map->layers_count; ++i) {
        layer = map->layers[i];
        if (strcmp(layer->name, "meta") == 0) {
            _level_read_meta(level, map, layer, tileset_meta);
        } else {
            _level_read_layer(level, map, layer, tileset_main);
        }
    }

    return level;
}

void
assets(AssetWrite *out)
{
    Palette palette;
    memset(&palette, 0, sizeof(Palette));
    // Skip color 0 as it's defined for COLOR_TRANSPARENT.
    palette.colors_count = 1;
    palette_read(&palette, assets_path(out, "palette.png"));

    Level *level = _level_read(assets_path(out, "map.json"));
    level->set_main = image_set_read(assets_path(out, "tileset.png"), &palette, 8, 8);

    assets_write(out, "res_image_set_main", &level->set_main->asset);

    char *player[] = {
        "player_walk",
        "player_idle",
        "player_brake",
        0
    };

    char name[1024];
    char path[1024];
    char **player_it = player;
    while (*player_it)
    {
        sprintf(name, "res_%s", *player_it);
        sprintf(path, "%s%s.png", out->path, *player_it);
        assets_write(out, name, image_set_read_ex(path, &palette, 8, 8, 4, 8, (V4i){{0}}));
        player_it++;
    }

    assets_write(out, "res_level_0_collision", level->map_collision);
    assets_write(out, "res_level_0_layer1", level->layers[0]);

    assets_write(out, "res_font", image_set_read_ex(assets_path(out, "font.png"), &palette, 4, 7, 0, 0, (V4i){{1, 0, 0, 0}}));
    assets_write(out, "res_icons", image_set_read_ex(assets_path(out, "icons.png"), &palette, 7, 7, 0, 0, (V4i){{0}}));

    assets_write(out, "res_palette", &palette);

//    assets_add_image_palette(out, "palette.png");
//    assets_add_image_set(out, "font.png", 4, 7, 0, 0, ((V4i){1, 0, 0, 0}));
//    assets_add_image_set(out, "icons.png", 7, 7, 0, 0, ((V4i){0}));
//    assets_add_image_set(out, "player_walk.png", 8, 8, 4, 8, ((V4i){0}));
//    assets_add_image_set(out, "player_idle.png", 8, 8, 4, 8, ((V4i){0}));
//    assets_add_image_set(out, "player_brake.png", 8, 8, 4, 8, ((V4i){0}));
//    assets_add_image_set(out, "tileset.png", 8, 8, 0, 0, ((V4i){0}));
//    assets_add_tile_map(out, "map.json");
}
