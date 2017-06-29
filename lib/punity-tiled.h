#ifndef TILED_HEADER_INCLUDED
#define TILED_HEADER_INCLUDED

#include "json.h"

typedef struct Tiled Tiled;
typedef struct TiledCacheEntry TiledCacheEntry;
typedef struct TiledScene TiledScene;
typedef struct TiledObject TiledObject;
typedef struct TiledItem TiledItem;
typedef struct TiledProperties TiledProperties;
typedef struct TiledProperty TiledProperty;

enum {
    TiledCache_Bitmap,
    TiledCache_Tileset
};

struct TiledCacheEntry
{
    char *path;
    Bitmap *bitmap;
};

struct Tiled
{
    TiledCacheEntry *cache;
    size_t cache_count;
    size_t cache_capacity;
};

struct TiledScene
{
    int width;
    int height;
    int tile_width;
    int tile_height;
    Color background;

    // Z-index sorted drawable items.
    TiledItem *items;
    TiledItem *items_end;
    size_t items_count;

#ifdef TILED_CUSTOM
    TILED_CUSTOM
#endif
};

#if 0
enum {
    TiledProperty_Boolean,
    TiledProperty_Integer,
    TiledProperty_Number,
    TiledProperty_String
};

struct TiledProperty
{
    const char *key;
    size_t key_length;
    int type;
    union {
        int i;
        float n;
        const char *s;
        bool b;
    };
    size_t s_length;
};

struct TiledProperties
{
    TiledProperty *properties;
    size_t properties_count;
};
#endif

enum {
    // Use `item.tilemap` to read.
    TiledType_TileMap,
    // Use `item.tile` to read.
    TiledType_Image,
    // Not using union properties.
    TiledType_Rectangle,

    // These are used for meta callback.

    // `data` passed is TiledScene struct.
    TiledType_Scene,
    // `data` passed is Tile struct.
    TiledType_Tile,
    // `data` passed is TiledItem struct.
    TiledType_Item,
};

struct TiledItem
{
    size_t size;
    int type;
    Rect rect;
    char *name;
    union {
        TileMap tilemap;
        Tile tile;
    };
#ifdef TILEDITEM_CUSTOM
    TILEDITEM_CUSTOM
#endif
};

// Callback that gets called for every new item or tile.
// You are required to read the meta information from json values passed and set them to `meta`.
#define TILED_CALLBACK(name) void name(int type, void *data, json_object_entry *properties, size_t properties_count, void *user)
typedef TILED_CALLBACK(TiledCallback);

void tiled_init(Tiled *T);
void tiled_free(Tiled *T);

// Loads tiled
void tiledscene_load_resource(Tiled *T, TiledScene *S, const char *resource_name,
    TiledCallback *meta_callback, void *meta_callback_user);
// Finds TiledItem by name.
// You can optionally pass `begin` to search from that.
TiledItem *tiledscene_find(TiledScene *S, const char *name, TiledItem *begin);
// Returns next item (doesn't check for boundaries).
TiledItem *tileditem_next(TiledItem *item);

// Does a simple linear-search for an entry with given key.
// `value` has to be json_object.
json_value *json_find_value(json_value *value, const char *key);

#endif // #ifndef TILED_HEADER_INCLUDED

#ifdef TILED_IMPLEMENTATION
#undef TILED_IMPLEMENTATION

#define JSON_IMPLEMENTATION
#include "json.h"

// Json tools.

static Color
json_color_(json_value *value) {
    // TODO: Read color from value->string.ptr.
    return color_make(0, 0, 0, 0);
}

json_value *
json_find_value(json_value *value, const char *key)
{
    ASSERT(value->type == json_object);
    json_object_entry *entry = value->object.values;
    for (uint i = 0; i != value->object.length; ++i, ++entry) {
        if (strcmp(entry->name, key) == 0) {
            return entry->value;
        }
    }
    return 0;
}


// TiledScene loader.
typedef struct
{
    int gid_begin;
    int gid_end;
    Tile *tiles;
}
TiledLoaderTileset_;


typedef struct
{
    i32 palette_range;
    Bank *storage;
    Tiled *tiled;
    TiledScene *scene;

    TiledLoaderTileset_ *tilesets;
    size_t tilesets_count;
    size_t tilesets_capacity;

    TiledCallback *meta_callback;
    void *meta_callback_user;
}
TiledLoader_;

static Bitmap *
tiled_load_bitmap_(TiledLoader_ *L, const char *path)
{
    // Look into the cache.
    for (uint i = 0; i != L->tiled->cache_count; ++i) {
        if (strcmp(path, L->tiled->cache[i].path) == 0) {
            return L->tiled->cache[i].bitmap;
        }
    }

    Bitmap *bitmap = bank_push_t(L->storage, Bitmap, 1);
    bitmap_load_resource_ex(L->storage, bitmap, path, L->palette_range);
    bitmap->tile_width  = bitmap->width;
    bitmap->tile_height = bitmap->height;

    // Add to the cache
    if (L->tiled->cache_count == L->tiled->cache_capacity) {
        L->tiled->cache_capacity += 16;
        L->tiled->cache = realloc(L->tiled->cache, sizeof(TiledCacheEntry) * L->tiled->cache_capacity);
    }
    TiledCacheEntry *entry = &L->tiled->cache[L->tiled->cache_count++];
    size_t path_length = strlen(path);
    entry->path = bank_push(L->storage, path_length + 1);
    memcpy(entry->path, path, path_length + 1);
    entry->bitmap = bitmap;

    return bitmap;
}

static TiledLoaderTileset_ *
tiled_add_tileset_(TiledLoader_ *L, int gid_first, int count)
{
    if (L->tilesets_count == L->tilesets_capacity) {
        L->tilesets_capacity += 16;
        L->tilesets = realloc(L->tilesets, sizeof(TiledLoaderTileset_) * L->tilesets_capacity);
    }
    TiledLoaderTileset_ *tileset = &L->tilesets[L->tilesets_count];
    L->tilesets_count++;

    tileset->gid_begin = gid_first;
    tileset->gid_end   = gid_first + count;
    tileset->tiles = malloc(count * sizeof(Tile));
    memset(tileset->tiles, 0, count * sizeof(Tile));
    
    return tileset;
}

static Tile *
tiled_find_tile_(TiledLoader_ *L, int gid)
{
    if (gid > 0) {
        TiledLoaderTileset_ *it = L->tilesets;
        for (int i = 0; i != L->tilesets_count; ++i, ++it) {
            if (gid >= it->gid_begin && gid < it->gid_end) {
                return it->tiles + (gid - it->gid_begin);
            }
        }
    }
    return 0;
}

static int
tiled_setup_tile_(TiledLoader_ *L, int gid, Tile *tile)
{
    Tile *tiled_tile = tiled_find_tile_(L, gid);
    if (tiled_tile) {
        *tile = *tiled_tile;
        return 1;
    }
    return 0;
}

static void
tiled_load_meta_(TiledLoader_ *L, int type, void *data, json_value *properties)
{
    if (!properties) {
        return;
    }
    
    json_value *v;
    switch (type)
    {
    case TiledType_Tile:
        {
            Tile *tile = (Tile*)data;
            v = json_find_value(properties, "edges");
            if (v && v->type == json_string)  {
                for (char *it = v->string.ptr; *it; ++it) {
                    switch (*it)
                    {
                    case 'T': case 't':
                        tile->flags |= Edge_Top;
                        break;
                    case 'B': case 'b':
                        tile->flags |= Edge_Bottom;
                        break;
                    case 'L': case 'l':
                        tile->flags |= Edge_Left;
                        break;
                    case 'R': case 'r':
                        tile->flags |= Edge_Right;
                        break;
                    }
                }
#ifdef TILEDTILE_DEFAULT_LAYER
                if (tile->flags != 0) {
                    tile->layer = TILEDTILE_DEFAULT_LAYER;
                }
#endif
            }

            v = json_find_value(properties, "layer");
            if (v && v->type == json_integer) {
                tile->layer = v->integer;
            }
            break;
        }
    }

    if (L->meta_callback) {
        L->meta_callback(type, data, properties->object.values, properties->object.length, L->meta_callback_user);
    }
}

static void
tiled_load_tileset_(TiledLoader_ *L, json_value *value, int *firstgid)
{
    json_value *v;
    TiledLoaderTileset_ *tiled_tileset;

    int gid_first;
    if (firstgid == 0) {
        v = json_find_value(value, "firstgid");
        ASSERT_MESSAGE(v, "`firstgid` not specified for tileset");
        gid_first = v->integer;
    } else {
        gid_first = *firstgid;
    }

    // Standard single-image tileset.
    json_value *value_image = json_find_value(value, "image");

    if (value_image)
    {
        Bitmap *tileset = tiled_load_bitmap_(L, value_image->string.ptr);

        v = json_find_value(value, "tilewidth");
        ASSERT_MESSAGE(v, "`tilewidth` not specified for tileset");
        tileset->tile_width = v->integer;

        v = json_find_value(value, "tileheight");
        ASSERT_MESSAGE(v, "`tileheight` not specified for tileset");
        tileset->tile_height = v->integer;

        uint tiles_count = (tileset->width / tileset->tile_width) * (tileset->height / tileset->tile_height);
        tiled_tileset = tiled_add_tileset_(L, gid_first, tiles_count);
        for (uint j = 0; j != tiles_count; ++j) {
            tiled_tileset->tiles[j].tileset = tileset;
            tiled_tileset->tiles[j].index = j;
        }

        // Load properties.
        v = json_find_value(value, "tileproperties");
        if (v)
        {
            json_object_entry *it_tile = v->object.values;
            for (uint j = 0; j != v->object.length; ++j, ++it_tile) {
                char *name_end = it_tile->name + it_tile->name_length;
                int index = strtol(it_tile->name, &name_end, 10);
                tiled_load_meta_(L, TiledType_Tile, &tiled_tileset->tiles[index], it_tile->value);
            }
        }
        return;
    }

    // Collection of images.
    json_value *value_tiles = json_find_value(value, "tiles");
    if (value_tiles)
    {
        tiled_tileset = tiled_add_tileset_(L, gid_first, value_tiles->object.length);
        json_object_entry *it_tile = value_tiles->object.values;
        for (uint j = 0; j != value_tiles->object.length; ++j, ++it_tile) {
            value_image = json_find_value(it_tile->value, "image");
            if (value_image) {
                Bitmap *tileset = tiled_load_bitmap_(L, value_image->string.ptr);
                char *name_end = it_tile->name + it_tile->name_length;
                int index = strtol(it_tile->name, &name_end, 10);
                tiled_tileset->tiles[index].tileset = tileset;
                tiled_tileset->tiles[index].index = index;
            }
        }

        return;
    }
}

static void
tiled_load_tilesets_(TiledLoader_ *L, json_value *value)
{
    json_value **it = value->array.values;
    for (uint i = 0; i != value->array.length; ++i, ++it)
    {
        json_value *value_source = json_find_value(*it, "source");
        if (value_source) {
            json_value *value_firstgid = json_find_value(*it, "firstgid");
            ASSERT_MESSAGE(value_firstgid, "invalid firstgid in reference to external tileset `%s`", value_source->string.ptr);
            // External tileset.
            size_t json_size;
            char *json = resource_get(value_source->string.ptr, &json_size);
            json_value *root = json_parse(json, json_size);
            ASSERT_MESSAGE(root, "invalid json in `%s`", value_source->string.ptr);
            ASSERT_MESSAGE(root->type == json_object, "root is not an object in `%s`", value_source->string.ptr);
            int firstgid = value_firstgid->integer;
            tiled_load_tileset_(L, root, &firstgid);
        } else {
            // Internal tileset.
            tiled_load_tileset_(L, *it, 0);
        }
    }
}

static TiledItem *
tiled_add_item_(TiledLoader_ *L, int type, const char *name, size_t name_length)
{
    if (L->scene->items == 0) {
        L->scene->items = (TiledItem*)L->storage->it;
    }

    TiledItem *item = bank_push(L->storage, sizeof(TiledItem) + name_length + 1);
    memset(item, 0, sizeof(TiledItem) + name_length + 1);
    item->name = (char*)(item + 1);
    if (name) {
        memcpy(item->name, name, name_length + 1);
    }
    
    item->type = type;
    item->size = sizeof(TiledItem) + name_length + 1;
    L->scene->items_count++;

    L->scene->items_end = (TiledItem*)L->storage->it;

    return item;
}

static void
tiled_load_tilemap_(TiledLoader_ *L, json_value *value)
{
    const char *name = 0;
    size_t name_length = 0;
    json_value *v = json_find_value(value, "name");
    if (v && v->type == json_string) {
        name = v->string.ptr;
        name_length = v->string.length;
    }
    TiledItem *item = tiled_add_item_(L, TiledType_TileMap, name, name_length);
    item->tilemap.tile_width  = L->scene->tile_width;
    item->tilemap.tile_height = L->scene->tile_height;

    json_object_entry *it;
    it = value->object.values;
    for (uint i = 0; i != value->object.length; ++i, ++it)
    {
        if (strcmp(it->name, "data") == 0) {
            item->tilemap.tiles = bank_push_t(L->storage, Tile, it->value->array.length);
            memset(item->tilemap.tiles, 0, sizeof(Tile) * it->value->array.length);
            Tile *tile = item->tilemap.tiles;
            Tile *tiled_tile;
            json_value **value_tile = it->value->array.values;
            for (uint i = 0; i != it->value->array.length; ++i, ++value_tile, ++tile) {
                tiled_setup_tile_(L, (*value_tile)->integer, tile);
            }
        } else if (strcmp(it->name, "width") == 0) {
            item->tilemap.width = item->rect.max_x = it->value->integer;
        } else if (strcmp(it->name, "height") == 0) {
            item->tilemap.height = item->rect.max_y = it->value->integer;
        }
    }

    tiled_load_meta_(L, TiledType_Item, item, json_find_value(value, "properties"));
    item->size += (item->tilemap.width * item->tilemap.height * sizeof(Tile));
}

static void
tiled_load_objects_(TiledLoader_ *L, json_value *value)
{
    json_value *root = json_find_value(value, "objects");
    if (!root) {
        return;
    }
    ASSERT_MESSAGE(root->type == json_array, "`objects` is not an array");

    uint i, j;
    json_value *v;
    json_object_entry *value_object_property;
    json_value **value_object = root->array.values;
    for (i = 0; i != root->array.length; ++i, ++value_object)
    {
        ASSERT_MESSAGE((*value_object)->type == json_object, "object expected");
        
        // Ellipses are ignored for now.
        v = json_find_value(*value_object, "ellipse");
        if (v) continue;

        // Polygons are ignored for now.
        v = json_find_value(*value_object, "polygon");
        if (v) continue;

        // Polylines are ignored for now.
        v = json_find_value(*value_object, "polyline");
        if (v) continue;

        const char *name = 0;
        size_t name_length = 0;
        json_value *v = json_find_value(*value_object, "name");
        if (v && v->type == json_string) {
            name = v->string.ptr;
            name_length = v->string.length;
        }
    
        TiledItem *item = tiled_add_item_(L, TiledType_Rectangle, name, name_length); 

        v = json_find_value(*value_object, "x");
        ASSERT_MESSAGE(v, "`x` coordinate not found");
        item->rect.min_x = v->integer;

        v = json_find_value(*value_object, "y");
        ASSERT_MESSAGE(v, "`y` coordinate not found");
        item->rect.min_y = v->integer;

        v = json_find_value(*value_object, "width");
        ASSERT_MESSAGE(v, "`width` coordinate not found");
        item->rect.max_x = item->rect.min_x + v->integer;

        v = json_find_value(*value_object, "height");
        ASSERT_MESSAGE(v, "`height` coordinate not found");
        item->rect.max_y = item->rect.min_y + v->integer;

        v = json_find_value(*value_object, "gid");
        if (v) {
            item->type = TiledType_Image;
            ASSERT_MESSAGE(tiled_setup_tile_(L, v->integer, &item->tile), "tile not found");
        }

        tiled_load_meta_(L, TiledType_Item, item, json_find_value(*value_object, "properties"));
    }
}

static void
tiled_load_layers_(TiledLoader_ *L, json_value *value)
{
    ASSERT_MESSAGE(value->type == json_array, "Layers are not an array.");
    
    // R->map->layers = bank_push_t(R->storage, TiledLayer*, value->array.length);
    // R->map->layers_count = value->array.length;

    json_value *v;
    json_value **value_layer = value->array.values;
    for (uint i = 0; i != value->array.length; ++i, ++value_layer)
    {
        v = json_find_value(*value_layer, "type");
        ASSERT_MESSAGE(v && v->type == json_string, "Layer type not found.");
        if (strcmp(v->string.ptr, "tilelayer") == 0) {
            tiled_load_tilemap_(L, *value_layer);
        } else if (strcmp(v->string.ptr, "objectgroup") == 0) {
            // TODO: Load objects.
            tiled_load_objects_(L, *value_layer);
        }
        // *layer = tiledreader_load_layer_(R, *value_layer);
    }
}

static void
tiled_load_scene_(TiledLoader_ *L, const char *json, size_t json_size)
{
    f64 t = perf_get();
    json_value *root = json_parse(json, json_size);
    LOG("json_parse %f\n", (perf_get() - t) * 1e3);
    ASSERT_MESSAGE(root, "Invalid JSON.");
    ASSERT_MESSAGE(root->type == json_object, "Root is not an object.");

    json_value *v;
    // Load tilesets first.
    v = json_find_value(root, "tilesets");
    if (v) {
        tiled_load_tilesets_(L, v);
    }

    v = json_find_value(root, "tilewidth");
    ASSERT_MESSAGE(v, "`tilewidth` is not set");
    L->scene->tile_width = v->integer;

    v = json_find_value(root, "tileheight");
    ASSERT_MESSAGE(v, "`tileheight` is not set");
    L->scene->tile_height = v->integer;

    json_object_entry *entry;
    entry = root->object.values;
    for (uint i = 0; i != root->object.length; ++i, ++entry)
    {
        if (strcmp(entry->name, "backgroundcolor") == 0) {
            L->scene->background = json_color_(entry->value);
        }
        else if (strcmp(entry->name, "width") == 0) {
            L->scene->width = entry->value->integer;
        }
        else if (strcmp(entry->name, "height") == 0) {
            L->scene->height = entry->value->integer;
        }
        else if (strcmp(entry->name, "layers") == 0) {
            tiled_load_layers_(L, entry->value);
        }
    }

    json_value_free(root);

    for (uint i = 0; i != L->tilesets_count; ++i) {
        free(L->tilesets[i].tiles);
    }
    free(L->tilesets);
}

//
// Public API.
//

void
tiled_init(Tiled *T)
{
    memset(T, 0, sizeof(Tiled));
}

void
tiled_free(Tiled *T)
{
    free(T->cache);
    T->cache_count = 0;
    T->cache_capacity = 0;
}

void
tiledscene_load_resource(Tiled *T, TiledScene *S, const char *resource_name,
    TiledCallback *meta_callback, void *meta_callback_user)
{
    TiledLoader_ L = {0};
    L.storage = CORE->storage;
    L.scene = S;
    L.tiled = T;
    L.meta_callback = meta_callback;
    L.meta_callback_user = meta_callback_user;
    memset(L.scene, 0, sizeof(TiledScene));

    size_t json_size;
    char *json = resource_get(resource_name, &json_size);
    tiled_load_scene_(&L, json, json_size);
}

TiledItem *
tileditem_next(TiledItem *item)
{
    return (TiledItem*)(((u8*)item) + item->size);
}

TiledItem *
tiledscene_find(TiledScene *S, const char *name, TiledItem *begin)
{
    size_t i = 0;
    TiledItem *it = S->items;
    if (begin) {
        ASSERT_MESSAGE(begin < S->items_end, "`begin` does not point to item in this array");
        it = begin;
    }
    for (; it != S->items_end; it = tileditem_next(it)) {
        if (strcmp(it->name, name) == 0) {
            return it;
        }
    }
    return 0;
}

#endif // TILED_IMPLEMENTATION
