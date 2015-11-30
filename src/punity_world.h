#ifndef WORLD_H
#define WORLD_H

#include "core/core.h"

#define WORLD_WIDTH (256)
#define WORLD_HEIGHT (256)
#define WORLD_COLLIDERS_MAX (256)
#define WORLD_CELL_SIZE (16)
#define WORLD_CELL_SIZE_SHIFT (4)
#define WORLD_COLLIDERS_PER_CELL_MAX (16)

// #define WORLD_ARRAY

#define WORLD_MAP
#define WORLD_HASH_MAP_BUCKETS_MAX (256)
#define WORLD_HASH_MAP_CELLS_MAX   (256)

#define world_hash(x, y) ((u32)(7*(x) + 3*(y)))
#define world_bucket(hash) ((hash) & (WORLD_HASH_MAP_BUCKETS_MAX - 1))

enum ColliderFlags
{
    ColliderFlag_LocalDisable = 0x01,
};

typedef struct Collider
{
    // This must be kept at start.
    union {
        struct {
            f32 x;
            f32 y;
            f32 w; // half the width
            f32 h; // half the height
        };
        V4f extents;
    };

    V4i cell; // NOTE: max_x and max_y are exclusive.
    void *ptr;
    u32 flags;
}
Collider;

struct WorldCell_;

typedef struct WorldCell_
{
    Collider *colliders[WORLD_COLLIDERS_PER_CELL_MAX];
    u8 colliders_count;
#ifdef WORLD_MAP
    i32 x, y;
    struct WorldCell_ *next;
#endif
}
WorldCell;

typedef struct World
{
#ifdef WORLD_ARRAY
    WorldCell cells[WORLD_WIDTH * WORLD_HEIGHT];
    s32 w;
    s32 h;
#endif
#ifdef WORLD_MAP
    WorldCell cells[WORLD_HASH_MAP_CELLS_MAX];
    u32 cells_count;
    WorldCell *buckets[WORLD_HASH_MAP_BUCKETS_MAX];
    V4i cell_rect;
#endif
    Collider colliders[WORLD_COLLIDERS_MAX];
    u32 colliders_count;
}
World;

#define WORLD_COLLISION_RETRY (4)

typedef struct Collision
{
    Collider *A;
    Collider *B;
    // Collision normal
    f32 nx;
    f32 ny;
    // Collision point
    f32 cx;
    f32 cy;
    // Movement delta
    f32 dx;
    f32 dy;
    // Collision t
    f32 t;

    f32 x;
    f32 y;

    Collider *_disabled[WORLD_COLLISION_RETRY];
}
Collision;

boolean on_collision(Collider *A, Collider *B, Collision *collision);

boolean collision_bounce(Collision *C) {
    f32 dp = v2_inner(C->dx, C->dy, C->nx, C->ny);
    C->dx -= 2 * dp * C->nx;
    C->dy -= 2 * dp * C->ny;
    return 1;
}

boolean collision_glide(Collision *C) {
    f32 dp = v2_inner(C->dx, C->dy, C->nx, C->ny);
    C->dx -= dp * C->nx;
    C->dy -= dp * C->ny;
    return 1;
}

void world_move(World *W, Collider *A, f32 dx, f32 dy, Collision *collision);
Collider *world_add(World *W, f32 x, f32 y, f32 w, f32 h, void *data);
void world_remove(World *W, Collider *collider);
WorldCell *world_cell(World *W, i32 x, i32 y);

#endif // WORLD_H
