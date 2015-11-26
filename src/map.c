#if 0
#include "core/core.h"

#define MAP_NODES_MAX (256)

typedef struct MapNode
{
    char *key;
    void *value;
    MapNode *next;
}
MapNode;

typedef struct
{
    memi nodes_count;
    memi buckets_count;
}
MapBase;

#define Map(KeyType, BucketsCount, NodesCount) \
    struct { \
        _Map map; \
        MapNode nodes[NodesCount]; \
        MapNode *buckets[BucketsCount]; \
        KeyType *_key_ref; \
        KeyType _key_val; \
    }

// http://www.partow.net/programming/hashfunctions/

internal void
_map_add_node(MapBase *map, )
{
    m
}

void
map_set_(MapBase *map, char *key, void *value)
{
    _map_add_node(key, )
}
#endif
