#include <string.h>

#include "punity_tiled.h"

#include <lib/json.h>
#include <lib/json.c>

typedef struct json_value_s json_value_s;
typedef struct json_number_s json_number_s;
typedef struct json_string_s json_string_s;
typedef struct json_object_s json_object_s;
typedef struct json_array_s json_array_s;
typedef struct json_object_element_s json_object_element_s;
typedef struct json_array_element_s json_array_element_s;

internal u32
json_u32(json_value_s *value) {
    ASSERT(value->type == json_type_number);
    json_number_s *x = (json_number_s*)value->payload;
    ASSERT(is_number(x->number[0]));
    char *end = x->number + x->number_size;
    return strtoul(x->number, &end, 10);
}

internal i32
json_i32(json_value_s *value) {
    ASSERT(value->type == json_type_number);
    json_number_s *x = (json_number_s*)value->payload;
    ASSERT(is_number(x->number[0]));
    char *end = x->number + x->number_size;
    return strtol(x->number, &end, 10);
}

internal json_string_s*
json_string(json_value_s *value) {
    ASSERT(value != 0 && value->type == json_type_string);
    return (json_string_s*)(value->payload);
}

internal boolean
json_string_eq(json_string_s *value, char *other) {
    return strcmp(value->string, other) == 0;
}

internal json_value_s *
json_find(json_object_s *object, char *key)
{
    json_object_element_s *it = object->start;
    while (it) {
        if (json_string_eq(it->name, key)) {
            return (json_value_s*)it->value;
        }
        it = it->next;
    }
    return 0;
}

internal void
_read_tile_map_tilesets(json_array_s *jtilesets, TiledTileMap *tile_map)
{
    json_array_element_s *jit = jtilesets->start;
    json_object_s *jtileset;
    json_object_element_s *jtileset_it;
    while (jit)
    {
        ASSERT(jit->value->type == json_type_object);

        TiledTileSet *tileset = calloc(sizeof(TiledTileSet), 1);
        tile_map->tilesets[tile_map->tilesets_count++] = tileset;

        jtileset = (json_object_s*)jit->value->payload;
        tileset->tiles_count = json_u32(json_find(jtileset, "tilecount"));

        jtileset_it = jtileset->start;
        while (jtileset_it)
        {
            if (json_string_eq(jtileset_it->name, "name")) {
                tileset->name = strdup((const char*)(json_string(jtileset_it->value)->string));

            } else if (json_string_eq(jtileset_it->name, "image")) {
                tileset->image = strdup(json_string(jtileset_it->value)->string);

            } else if (json_string_eq(jtileset_it->name, "tilewidth")) {
                tileset->tile_w = json_u32(jtileset_it->value);

            } else if (json_string_eq(jtileset_it->name, "tileheight")) {
                tileset->tile_h = json_u32(jtileset_it->value);

            } else if (json_string_eq(jtileset_it->name, "firstgid")) {
                tileset->gid_begin = json_u32(jtileset_it->value);
                tileset->gid_end = tileset->gid_begin + tileset->tiles_count;
            }
            jtileset_it = jtileset_it->next;
        }
        jit = jit->next;
    }
}

internal void
_read_tile_map_cell(TiledTileMap *tile_map, u32 gid, TiledCell *cell)
{
    cell->gid = gid;

    if (gid == 0) {
        cell->tileset = 0;
        cell->tile = 0;
        return;
    }

    TiledTileSet *tileset;
    for (u32 i = 0; i != tile_map->tilesets_count; ++i) {
        tileset = tile_map->tilesets[i];
        if (gid >= tileset->gid_begin && gid < tileset->gid_end) {
            cell->tileset = tileset;
            cell->tile = gid - tileset->gid_begin; // + 1;
            return;
        }
    }

    ASSERT(0 && "Tileset for `gid` not found.");
}

internal void
_read_tile_map_layers(json_array_s *jlayers, TiledTileMap *tile_map)
{
    TiledCell *cell;
    json_array_element_s *jit = jlayers->start;
    json_object_s *jlayer;
    while (jit)
    {
        ASSERT(jit->value->type == json_type_object);
        jlayer = (json_object_s*)jit->value->payload;
        if (json_string_eq(json_string(json_find(jlayer, "type")), "tilelayer"))
        {
            TiledTileLayer *layer = calloc(sizeof(TiledTileLayer), 1);
            tile_map->layers[tile_map->layers_count++] = layer;

            json_object_element_s *jlayer_it = jlayer->start;
            while (jlayer_it)
            {
                if (json_string_eq(jlayer_it->name, "name")) {
                    layer->name = strdup(json_string(jlayer_it->value)->string);

                } else if (json_string_eq(jlayer_it->name, "width")) {
                    layer->w = json_i32(jlayer_it->value);

                } else if (json_string_eq(jlayer_it->name, "height")) {
                    layer->h = json_i32(jlayer_it->value);

                } else if (json_string_eq(jlayer_it->name, "x")) {
                    layer->x = json_i32(jlayer_it->value);

                } else if (json_string_eq(jlayer_it->name, "y")) {
                    layer->y = json_i32(jlayer_it->value);

                } else if (json_string_eq(jlayer_it->name, "data")) {
                    json_array_s *jcells = (json_array_s *)jlayer_it->value->payload;
                    layer->cells = calloc(sizeof(TiledCell) * jcells->length, 1);
                    cell = layer->cells;
                    json_array_element_s *jdata_it = jcells->start;
                    while (jdata_it) {
                        _read_tile_map_cell(tile_map, json_u32(jdata_it->value), cell);
                        cell++;
                        jdata_it = jdata_it->next;
                    }
                }
                jlayer_it = jlayer_it->next;
            }
        }
        jit = jit->next;
    }
}

internal void
_read_tile_map_root(json_object_s *json_root, TiledTileMap *tile_map)
{
    json_object_element_s *it = json_root->start;
    json_array_s *layers = 0;
    while (it)
    {
        if (json_string_eq(it->name, "name")) {
            tile_map->name = strdup(json_string(it->value)->string);

        } else if (json_string_eq(it->name, "width")) {
            tile_map->w = json_u32(it->value);

        } else if (json_string_eq(it->name, "height")) {
            tile_map->h = json_u32(it->value);

        } else if (json_string_eq(it->name, "tilewidth")) {
            tile_map->tile_w = json_u32(it->value);

        } else if (json_string_eq(it->name, "tileheight")) {
            tile_map->tile_h= json_u32(it->value);

        } else if (json_string_eq(it->name, "layers")) {
            ASSERT(it->value->type == json_type_array);
            layers = (json_array_s*)it->value->payload;

        } else if (json_string_eq(it->name, "tilesets")) {
            ASSERT(it->value->type == json_type_array);
            _read_tile_map_tilesets((json_array_s*)it->value->payload, tile_map);
        }
        it = it->next;
    }

    // Has to be done after the tilesets are loaded.
    ASSERT(layers);
    _read_tile_map_layers(layers, tile_map);
}

TiledTileMap *
tiled_read(char *path)
{
    TiledTileMap *tile_map = calloc(sizeof(TiledTileMap), 1);

    IOReadResult file = io_read(path);
    ASSERT(file.data);

    json_value_s* value = json_parse(file.data, file.size);
    ASSERT(value->type == json_type_object);
    _read_tile_map_root(value->payload, tile_map);

    return tile_map;
}
