#ifndef WORLD_H
#define WORLD_H

#include "punity_core.h"

#define WORLD_WIDTH (256)
#define WORLD_HEIGHT (256)
#define WORLD_COLLIDERS_MAX (256)
#define WORLD_CELL_SIZE (16)
#define WORLD_CELL_SIZE_SHIFT (4)
#define WORLD_TILE_SIZE (8)
#define WORLD_TILE_SIZE_SHIFT (3)

#define world_cell_to_tile(cell) \
    ((cell) << 1)

#define WORLD_COLLIDERS_PER_CELL_MAX (16)

#define WORLD_HASH_MAP_BUCKETS_MAX (256)
#define WORLD_HASH_MAP_CELLS_MAX   (256)

#define WORLD_COLLISION_EPSILON (0.01f)

#define world_hash(x, y) ((u32)(7*(x) + 3*(y)))
#define world_bucket(hash) ((hash) & (WORLD_HASH_MAP_BUCKETS_MAX - 1))

enum ColliderFlags
{
    ColliderFlag_LocalDisable = 0x01,
    ColliderFlag_TileMap = 0x02,
    ColliderFlag_Collider = 0x03,
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
    u8 flags;
    u8 edges;
}
Collider;

struct WorldCell_;

typedef struct WorldCell_
{
    Collider *colliders[WORLD_COLLIDERS_PER_CELL_MAX];
    u8 colliders_count;
    i32 x, y;
    struct WorldCell_ *next;
}
WorldCell;

typedef struct World
{
    WorldCell cells[WORLD_HASH_MAP_CELLS_MAX];
    u32 cells_count;
    WorldCell *buckets[WORLD_HASH_MAP_BUCKETS_MAX];
// #ifdef _DEBUG
    u32 buckets_cells_count[WORLD_HASH_MAP_BUCKETS_MAX];
// #endif
    WorldCell *cell_pool;
    V4i cell_rect;
    Collider colliders[WORLD_COLLIDERS_MAX];
    u32 colliders_count;
    TileMap *tile_map;
    V4i tile_rect;
}
World;

#define WORLD_COLLISION_RETRY (4)
#define WORLD_COLLISION_IGNORE (4)

typedef struct Collision
{
    Collider *A;
    Collider *B;
    // Movement collision (end) point.
    f32 end_x;
    f32 end_y;
    // Movement begin point.
//    f32 begin_x;
//    f32 begin_y;
    // Movement overshoot point.
    // Where collider would have ended should it not collide.
    // This is used in glide and bounce.
    f32 over_x;
    f32 over_y;
    // Movement delta (from collision begin position).
    f32 delta_x;
    f32 delta_y;
    // Collision t.
    f32 t;
    // Collision normal.
    f32 normal_x;
    f32 normal_y;

    Collider *_ignored[WORLD_COLLISION_IGNORE];
}
Collision;

typedef enum OnCollisionResult
{
    // Collision checking continues.
    // The callback has set dx and dy members
    // that are used to continue.
    // This is useful for gliding and bouncing.
    OnCollisionResult_Continue,
    // Collision is ignored.
    // This is useful for triggers.
    // B is flagged as temporarily ignored.
    // Retry counter is reset to it's normal value.
    OnCollisionResult_Ignore,
    // Collision checking stops immediately.
    // This is useful when A dies.
    OnCollisionResult_Stop
}
OnCollisionResult;

OnCollisionResult collision_bounce(Collider *A, Collider *B, Collision *C);
OnCollisionResult collision_glide(Collider *A, Collider *B, Collision *C);
OnCollisionResult collision_ignore(Collider *A, Collider *B, Collision *C);
OnCollisionResult collision_stop(Collider *A, Collider *B, Collision *C);

#define WORLD_ON_COLLISION(name) OnCollisionResult name(Collider *A, Collider *B, Collision *C)
typedef WORLD_ON_COLLISION(OnCollision);

typedef enum CollisionTestMask
{
    CollisionTestMask_TileMap       = 0x01,
    CollisionTestMask_Colliders     = 0x02,

    CollisionTestMask_All           = 0xFF
}
CollisionTestMask;

// Not re-entrant.
// Writes temporary flag ColliderFlag_LocalDisable to tested colliders.
Collision *world_test_move(World *W, Collider *A, f32 dx, f32 dy, u8 mask, Collision *collision, OnCollision* on_collision);
void world_move(World *W, Collider *A, f32 x, f32 y);
Collider *world_add(World *W, f32 x, f32 y, f32 w, f32 h, void *data);
void world_remove(World *W, Collider *collider);
WorldCell *world_cell(World *W, i32 x, i32 y);

#endif // WORLD_H
