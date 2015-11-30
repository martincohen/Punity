#ifndef PUNITY_TILED_H
#define PUNITY_TILED_H

#include "punity_core.h"

typedef struct TiledProperty
{
    char *key;
    char *value;
}
TiledProperty;

typedef struct TiledTile
{
    TiledProperty *properties;
    u32 properties_count;
}
TiledTile;

typedef struct TiledTileSet
{
    char *name;
    char *image;
    u32 tile_w;
    u32 tile_h;
    TiledTile *tiles;
    u32 tiles_count;
    TiledProperty *properties;
    u32 properties_count;

    u32 gid_begin;
    u32 gid_end;
}
TiledTileSet;

typedef struct TiledCell
{
    TiledTileSet *tileset;
    u32 tile;
    u32 gid; // 0 - if empty
}
TiledCell;

typedef struct TiledTileLayer
{
    char *name;

    i32 x;
    i32 y;
    i32 w;
    i32 h;
    TiledCell *cells;
}
TiledTileLayer;

typedef struct TiledTileMap
{
    char *name;

    u32 tile_w;
    u32 tile_h;
    u32 w;
    u32 h;

    TiledTileLayer *layers[256];
    u32 layers_count;
    TiledTileSet *tilesets[256];
    u32 tilesets_count;
}
TiledTileMap;

TiledTileMap *tiled_read(char *path);


#endif // PUNITY_TILED_H
