#include "lib/punity/punity_tiled.c"

// TODO: If main() gets argument "update-assets" it should call assets().
// TODO: Encode the target storage paths better.

// IDs corresponding to Tiled's tile ID.
#define META_TILE_SOLID (0)
#define META_TILE_PLAYER (16)

typedef enum
{
    TileFlag_Solid = 0x01,
}
TileFlags;

// There will be two tilesets:
//   1) meta
//   2) visual
//
// Meta is thrown away and not exported.
// Visual is a merged tileset of all the visual tilesets (common + levels).
// So to merge the tileset, we will first have to:
// 1) Walk through all the Tiled's JSONs
// 2) Build one list of all the tilesets we're using
// 3) Merge it down keeping track of Tiled's GIDs mapped to merged IDs.
//    - This should also throw away any empty tiles in tilesets.
//    - Possibly also throw away any unused tiles as well?

typedef struct
{
    // ImageSet *image_set;
    TiledTileSet *tiled_tileset;
    u32 *tile_flags;
    u32 *tile_ids;
    u32 tile_ids_begin;
}
TileSet;

typedef struct
{
    AssetWrite *out;
    Palette *palette;

    // Separate tilesets converted from Tiled.
    Memory tilesets;
    // Linear cells for the final compiled tileset.
    Memory final_tileset_cells;
    u32 final_tileset_cells_count;

    // Current level info.
    TiledTileMap *level_tiled_map;
    Level *level;
    TileMap *level_map_collision;
    TileMap *level_map_meta;
    TiledTileSet *level_tileset_meta;
}
Context;

internal TileMap *
_map_new(u32 w, u32 h)
{
    memi size = tile_map_size(w, h);
    TileMap *map = calloc(size, 1);
    map->asset.size = size;
    map->asset.type = AssetType_TileMap;
    map->w = w;
    map->h = h;
    return map;
}

internal void
context_add_meta_layer(Context *context, TiledTileLayer *tiled_layer)
{
    TiledTileMap *map = context->level_tiled_map;
    ASSERT(context->level_tileset_meta);
    ASSERT((u32)tiled_layer->w == map->w && (u32)tiled_layer->h == map->h);

    TiledCell *cell;
    u32 x, y;
    for (y = 0; y != map->h; ++y) {
        for (x = 0; x != map->w; ++x) {
            cell = tiled_layer->cells + (x + y * map->w);
            if (cell->gid)
            {
                ASSERT(cell->tileset == context->level_tileset_meta);

                switch (cell->tile)
                {
                case META_TILE_SOLID:
                    tile_map_data(context->level_map_collision)[x + y * map->w] = 1;
                    break;
                case META_TILE_PLAYER:
                    context->level->player_start.x = x;
                    context->level->player_start.y = y;
                    break;
                }
            }
        }
    }
}

internal TileMap*
context_add_layer(Context *context, TiledTileLayer *tiled_layer)
{
    ASSERT(context->level->layers_count != LEVEL_LAYERS_MAX);

    u32 layer_index = context->level->layers_count;

    TiledTileMap *map = context->level_tiled_map;
    ASSERT(tiled_layer->w == (i32)map->w && tiled_layer->h == (i32)map->h);
    TileMap *layer = _map_new(map->w, map->h);
    layer->w = map->w;
    layer->h = map->h;
    TiledCell *cell;
    TileSet *tileset;
    i32 x, y;
    u32 tile_id;
    for (y = 0; y != tiled_layer->h; ++y) {
        for (x = 0; x != tiled_layer->w; ++x) {
            cell = tiled_layer->cells + x + y * tiled_layer->w;
            if (cell->tile && cell->tileset != context->level_tileset_meta)
            {
                ASSERT(cell->tileset);
                ASSERT(cell->tile < 0x10000);
                ASSERT(cell->tile < (cell->tileset->tiles_count));

                tileset = (TileSet*)cell->tileset->user;
                tile_id = tileset->tile_ids[cell->tile];
                ASSERT(tile_id < 0x10000);
                tile_map_data(layer)[x + y * layer->w] = (u16)tile_id;

                if (tile_id != TILE_EMPTY)
                {
                    if (tileset->tile_flags[tile_id - tileset->tile_ids_begin] & TileFlag_Solid) {
                        tile_map_data(context->level_map_collision)[x + y * map->w] = 1;
                    }
                }
            }
            else
            {
                tile_map_data(layer)[x + y * layer->w] = TILE_EMPTY;
            }
        }
    }

    // Properties
    char *end;
    LayerProperties *layer_properties = &context->level->layer_properties[layer_index];
    layer_properties->parallax = 0;
    TiledProperty *property;
    for (u32 i = 0; i != tiled_layer->properties.properties_count; ++i)
    {
        property = &tiled_layer->properties.properties[i];
        if (token_equal_0(&property->key, "parallax")) {
            end = property->value.end;
            layer_properties->parallax = strtol(property->value.begin, &end, 10);
            layer_properties->parallax = MAX(-5, MIN(5, layer_properties->parallax));
        }
    }

    context->level->layers[layer_index] = assets_add(context->out, 0, &layer->asset);
    context->level->layers_count++;

    return layer;
}

TileSet *
context_find_tileset(Context *context, TiledTileSet *tiled_tile_set)
{
    TileSet *it = (TileSet *)context->tilesets.begin;
    while (it != (TileSet *)context->tilesets.it)
    {
        if (strcmp(it->tiled_tileset->image, tiled_tile_set->image) == 0) {
            return it;
        }
        it++;
    }
    return 0;
}

internal TileSet *
context_add_tileset(Context *context, TiledTileSet *tiled_tileset)
{
    TileSet *tileset = context_find_tileset(context, tiled_tileset);
    if (!tileset)
    {
        if (strcmp(tiled_tileset->name, "meta") == 0) {
            ASSERT_MESSAGE(context->level_tileset_meta == 0, "Meta tileset was used twice in a single map.");
            context->level_tileset_meta = tiled_tileset;
            return 0;
        }

        assets_add_path(context->out, assets_path(context->out, tiled_tileset->image));
        TileSet *tileset = memory_push(&context->tilesets, TileSet, 1, 16);
        memset(tileset, 0, sizeof(TileSet));
        tileset->tiled_tileset = tiled_tileset;
        tiled_tileset->user = tileset;

        // Read image set.
        ImageSet *image_set = image_set_read(assets_path(context->out, tiled_tileset->image),
                                             context->palette,
                                             tiled_tileset->tile_w,
                                             tiled_tileset->tile_h);

        // Walk through the image set sprites, remove empty ones.
        // TODO: Also only save the effective rectangles.
        tileset->tile_ids = calloc(sizeof(u32), image_set->count);
        tileset->tile_ids_begin = context->final_tileset_cells_count;

        u8 *final_cell_pixels;
        u8 *cell_pixels;

        u32 cell_size = image_set->cw * image_set->ch;
        u32 optimized_cells_count = 0;
        V4i effective_rect;
        for (u32 i = 0; i != image_set->count; ++i)
        {
            cell_pixels = image_set_cell(image_set, i);
            if (image_effective_rect(cell_pixels, image_set->cw, image_set->ch, &effective_rect))
            {
                final_cell_pixels = memory_push(&context->final_tileset_cells, u8, cell_size, kilobytes(64));
                memcpy(final_cell_pixels, cell_pixels, cell_size);

                tileset->tile_ids[i] = context->final_tileset_cells_count;
                context->final_tileset_cells_count++;
                optimized_cells_count++;
            } else {
                tileset->tile_ids[i] = TILE_EMPTY;
            }
        }
        image_set_free(image_set);

#if 0
        tileset->image_set = image_set_make(final_cell_pixels, optimized_cells_count,
                                            image_set->cw, image_set->ch,
                                            image_set->path);
#endif

        tileset->tile_flags = calloc(sizeof(u32), optimized_cells_count);

        // Read tile flags.
        TiledTile *tile;
        u32 tile_id;
        for (u32 i = 0; i != tiled_tileset->tiles_count; ++i)
        {
            // Skip unmapped cells.
            tile_id = tileset->tile_ids[i];
            if (tile_id != TILE_EMPTY)
            {
                tile_id -= tileset->tile_ids_begin;
                ASSERT(tile_id < optimized_cells_count);
                tile = &tiled_tileset->tiles[i];
                tileset->tile_flags[tile_id] = 0;
                TiledProperty *properties = tile->properties.properties;
                for (u32 j = 0; j != tile->properties.properties_count; j++)
                {
                    if (token_equal_0(&properties[j].key, "solid") &&
                        token_equal_0(&properties[j].value, "1")) {
                        tileset->tile_flags[tile_id] |= TileFlag_Solid;
                    }
                }
            }
        }
    }

    return tileset;
}

internal void
context_add_tilesets(Context *context)
{
    TiledTileSet *tiled_tileset;
    for (u32 i = 0; i != context->level_tiled_map->tilesets_count; ++i) {
        tiled_tileset = context->level_tiled_map->tilesets[i];
        context_add_tileset(context, tiled_tileset);
    }
}

internal ImageSet *
context_compile_tileset(Context *context)
{
    return image_set_make(context->final_tileset_cells.begin,
                          memory_count(&context->final_tileset_cells, u8),
                          8, 8,
                          0);
}

internal Level *
context_add_level(Context *context, char *path)
{
    context->level = calloc(sizeof(Level), 1);
    context->level->asset.size = sizeof(Level);
    context->level->asset.type = AssetType_Level;
    context->level->asset.path = string_duplicate(path);
    context->level_tileset_meta = 0;

    TiledTileMap *map = context->level_tiled_map = tiled_read(path);

    context->level_map_collision = _map_new(map->w, map->h);
    context->level_map_meta = _map_new(map->w, map->h);

    context_add_tilesets(context);

    u32 i;

    u32 layer_index = 0;
    TiledTileLayer *tiled_layer;
    for (i = 0; i != map->layers_count; ++i) {
        tiled_layer = map->layers[i];
        if (strcmp(tiled_layer->name, "meta") == 0) {
            context_add_meta_layer(context, tiled_layer);
        } else {
            context_add_layer(context, tiled_layer);
            if (strcmp(tiled_layer->name, "main") == 0) {
                context->level->layer_main = layer_index;
            } else if (strcmp(tiled_layer->name, "parallax") == 0) {
                context->level->layer_parallax = layer_index;
            }
            layer_index++;
        }
    }

    // Optimize collision map.

    u32 x, y;
    u16 cell;
    for (y = 0; y != context->level_map_collision->h; ++y) {
        for (x = 0; x != context->level_map_collision->w; ++x) {
            if (tile_map_cell(context->level_map_collision, x, y) == 1)
            {
                cell = 0;

                cell |= (tile_map_cell(context->level_map_collision, x-1, y) == 0) * DirectionFlag_Left;
                cell |= (tile_map_cell(context->level_map_collision, x, y-1) == 0) * DirectionFlag_Top;
                cell |= (tile_map_cell(context->level_map_collision, x+1, y) == 0) * DirectionFlag_Right;
                cell |= (tile_map_cell(context->level_map_collision, x, y+1) == 0) * DirectionFlag_Bottom;

                if (cell != 0) {
                    tile_map_data(context->level_map_collision)[x + y * map->w] = cell;
                }
            }
        }
    }

    context->level->map_collision = assets_add(context->out, 0, &context->level_map_collision->asset);
    context->level->map_meta = assets_add(context->out, 0, &context->level_map_meta->asset);

    free(map);

    return context->level;
}

void
assets(AssetWrite *out)
{
    assets_add_type(out, Level);

    Context context;
    memset(&context, 0, sizeof(Context));
    context.out = out;

    context.palette = calloc(sizeof(Palette), 1);
    // Skip color 0 as it's defined for COLOR_TRANSPARENT.
    context.palette->colors_count = 1;

    StringListBuilder files_builder;
    memset(&files_builder, 0, sizeof(StringListBuilder));
    out->files = &files_builder;

    palette_read(context.palette, assets_path(out, "palette.png"));

    // TODO: Read all the maps here.
    assets_add(out, "res_level_1", (Asset*)context_add_level(&context, assets_path(out, "map.json")));

    assets_add(context.out, "res_tileset", (Asset*)context_compile_tileset(&context));

    //
    // Misc
    //

    assets_add(out, "res_font",
               (Asset*)image_set_read_ex(assets_path(out, "font.png"),
                                         context.palette, 4, 7, 0, 0,
                                         (V4i){{1, 0, 0, 0}})
               );

    assets_add(out, "res_icons",
               (Asset*)image_set_read_ex(assets_path(out, "icons.png"),
                                         context.palette, 7, 7, 0, 0,
                                         (V4i){{0}})
               );

    //
    // Player
    //

    char *player[] = {
        "player_walk",
        "player_idle",
        "player_brake",
        0
    };

    char var[1024];
    char file[1024];
    char **player_it = player;
    while (*player_it)
    {
        sprintf(var, "res_%s", *player_it);
        sprintf(file, "%s.png", *player_it);
        assets_add(out, var,
                   (Asset*)image_set_read_ex(assets_path(out, file),
                                             context.palette, 8, 8, 4, 8,
                                             (V4i){{0}})
                   );
        player_it++;
    }

    //
    // Sounds
    //

    assets_add(out, "res_sound_music", (Asset*)sound_read(assets_path(out, "sounds/game.ogg")));
    assets_add(out, "res_sound_jump", (Asset*)sound_read(assets_path(out, "sounds/jump.ogg")));
    assets_add(out, "res_sound_step", (Asset*)sound_read(assets_path(out, "sounds/step1.ogg")));

    //
    // Palette
    //

    assets_add(out, "res_palette", &context.palette->asset);

#ifdef _DEBUG
    StringList *files = string_list_make(out->files);
    assets_add(out, "res_files", (Asset*)files);
#endif
}
