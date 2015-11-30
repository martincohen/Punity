#include "punity_core.h"
#include "punity_world.h"

// TODO: Better way to test for triggers / ignorable colliders.
//     - Option is to simplify and only allow tile map colliders to be ignored.
//     - Other option is to store the latest collisions in linked-list and operate
//       over that.

OnCollisionResult
collision_bounce(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);

    f32 dx = C->over_x - C->end_x;
    f32 dy = C->over_y - C->end_y;
    f32 dp = v2_inner(dx, dy, C->normal_x, C->normal_y);
    C->delta_x = dx - 2 * dp * C->normal_x;
    C->delta_y = dy - 2 * dp * C->normal_y;
    return OnCollisionResult_Continue;
}

OnCollisionResult
collision_glide(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);

    f32 dx = C->over_x - C->end_x;
    f32 dy = C->over_y - C->end_y;
    f32 dp = v2_inner(dx, dy, C->normal_x, C->normal_y);
    C->delta_x = dx - dp * C->normal_x;
    C->delta_y = dy - dp * C->normal_y;
    return OnCollisionResult_Continue;
}

OnCollisionResult
collision_stop(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);
    unused(C);

    return OnCollisionResult_Stop;
}

OnCollisionResult
collision_ignore(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);
    unused(C);

    return OnCollisionResult_Ignore;
}

void
world_cell_clip(V4i *r, V4i *clip)
{
    r->min_x = MAX(clip->min_x, MIN(clip->max_x, r->min_x));
    r->min_y = MAX(clip->min_y, MIN(clip->max_y, r->min_y));
    r->max_x = MAX(r->min_x,    MIN(clip->max_x, r->max_x));
    r->max_y = MAX(r->min_y,    MIN(clip->max_y, r->max_y));
}

V4i
world_collider_to_cell(f32 x, f32 y, f32 w, f32 h, i32 cell_size)
{
    V4i result;

    result.min_x = (i32)(x - w) / cell_size;
    result.min_y = (i32)(y - h) / cell_size;
    result.max_x = (i32)(x + w) / cell_size + 1;
    result.max_y = (i32)(y + h) / cell_size + 1;

    return result;
}

V4i
world_cell_range(f32 x1, f32 y1, f32 w, f32 h, f32 dx, f32 dy, i32 cell_size)
{
    f32 x2 = x1 + dx;
    f32 y2 = y1 + dy;

    V4i result;
    result.min_x = (i32)(MIN(x1, x2) - w - (cell_size * 0.5f)) / cell_size;
    result.min_y = (i32)(MIN(y1, y2) - h - (cell_size * 0.5f)) / cell_size;
    result.max_x = (i32)(MAX(x1, x2) + w + (cell_size * 0.5f)) / cell_size + 1;
    result.max_y = (i32)(MAX(y1, y2) + h + (cell_size * 0.5f)) / cell_size + 1;

    return result;
}

#if 0
V4i
world_rect_to_cell(World *W, V4f *c)
{
    unused(W);

    V4i result;

    result.min_x =  (i32)(c->x - c->w) / WORLD_CELL_SIZE;
    result.min_y =  (i32)(c->y - c->h) / WORLD_CELL_SIZE;
    result.max_x = ((i32)(c->x + c->w) / WORLD_CELL_SIZE) + 1;
    result.max_y = ((i32)(c->y + c->h) / WORLD_CELL_SIZE) + 1;

    return result;
}
#endif


internal inline WorldCell*
_world_new_cell(World *W, i32 x, i32 y, WorldCell *next)
{
    WorldCell *cell = W->cell_pool;
    if (cell) {
        W->cell_pool = cell->next;
    } else {
        ASSERT(W->cells_count != WORLD_HASH_MAP_CELLS_MAX);
        cell = W->cells + W->cells_count;
        W->cells_count++;
    }

    cell->colliders_count = 0;
    cell->x = x;
    cell->y = y;
    cell->next = next;
    return cell;
}

internal void
_world_cell_remove_collider(World *W, WorldCell *cell, u32 index, WorldCell *previous, WorldCell **bucket)
{
    if (cell->colliders_count != 1) {
        cell->colliders[index] = cell->colliders[cell->colliders_count - 1];
    }
    cell->colliders_count--;

    if (cell->colliders_count == 0)
    {
        if (previous) {
            previous->next = cell->next;
        } else if (cell->next == 0){
            // No previous, or next. Empty the bucket.
            *bucket = 0;
        }
        W->buckets_cells_count[bucket - W->buckets]--;
        cell->next = W->cell_pool;
        W->cell_pool = cell;
    }
}

internal void
_world_cell_remove(World *W, V4i *range, Collider *collider)
{
    i32 x, y;
    u32 i;
    WorldCell *cell, *previous, **bucket;
    for (y = range->min_y; y != range->max_y; ++y) {
        for (x = range->min_x; x != range->max_x; ++x) {

            bucket = W->buckets + world_bucket(world_hash(x, y));
            cell = *bucket;
            previous = 0;
            while (cell && (cell->x != x || cell->y != y)) {
                previous = cell;
                cell = cell->next;
            }

            if (cell)
            {
                do {
                    for (i = 0; i != cell->colliders_count; ++i) {
                        if (cell->colliders[i] == collider) {
                            _world_cell_remove_collider(W, cell, i, previous, bucket);
                            goto next;
                        }
                    }
                    cell = cell->next;
                } while (cell && (cell->x == x && cell->y == y));
            }
next:;
        }
    }
}

internal void
_world_cell_add(World *W, V4i *range, Collider *collider)
{
    W->cell_rect.min_x = MIN(W->cell_rect.min_x, range->min_x);
    W->cell_rect.min_y = MIN(W->cell_rect.min_y, range->min_y);
    W->cell_rect.max_x = MAX(W->cell_rect.max_x, range->max_x);
    W->cell_rect.max_y = MAX(W->cell_rect.max_y, range->max_y);

    i32 x, y;
    WorldCell **bucket;
    WorldCell *cell;
    for (y = range->min_y; y != range->max_y; ++y) {
        for (x = range->min_x; x != range->max_x; ++x) {

            bucket = W->buckets + world_bucket(world_hash(x, y));
            cell = *bucket;

            while (cell && (cell->x != x || cell->y != y)) {
                cell = cell->next;
            }

            if (!cell) {
                cell = *bucket = _world_new_cell(W, x, y, *bucket);
            } else if (cell->colliders_count == WORLD_COLLIDERS_PER_CELL_MAX) {
                cell = _world_new_cell(W, x, y, cell);
            }
            W->buckets_cells_count[bucket - W->buckets]++;

            ASSERT(cell->colliders_count != WORLD_COLLIDERS_PER_CELL_MAX);
            cell->colliders[cell->colliders_count] = collider;
            cell->colliders_count++;
        }
    }
}

WorldCell *
world_cell(World *W, i32 x, i32 y)
{
    if (x >= W->cell_rect.min_x && x < W->cell_rect.max_x &&
        y >= W->cell_rect.min_y && y < W->cell_rect.max_y)
    {
        u32 bucket = world_bucket(world_hash(x, y));
        WorldCell *cell = W->buckets[bucket];
        while (cell && (cell->x != x || cell->y != y)) {
            ++cell;
        }
        return cell;
    }
    return 0;
}

internal void
_world_cell_update(World *W, Collider *collider)
{
    V4i cell = world_collider_to_cell(collider->x, collider->y,
                                      collider->w, collider->h,
                                      WORLD_CELL_SIZE);

    if (memcmp(&collider->cell, &cell, sizeof(V4i)) != 0) {
        _world_cell_remove(W, &collider->cell, collider);
        _world_cell_add(W, &cell, collider);
        collider->cell = cell;
    }
}

Collider *
world_add(World *W, f32 x, f32 y, f32 w, f32 h, void *ptr)
{
    ASSERT(W->colliders_count != WORLD_COLLIDERS_MAX);

    Collider *collider = W->colliders + W->colliders_count;
    W->colliders_count++;

    collider->flags = ColliderFlag_Collider;
    collider->x = x;
    collider->y = y;
    collider->w = w;
    collider->h = h;
    collider->ptr = ptr;
    collider->cell = world_collider_to_cell(collider->x, collider->y,
                                            collider->w, collider->h,
                                            WORLD_CELL_SIZE);
    collider->flags = 0;

    _world_cell_add(W, &collider->cell, collider);

    return collider;
}

void
world_remove(World *W, Collider *collider)
{
    ASSERT(W->colliders_count);

    _world_cell_remove(W, &collider->cell, collider);
    W->colliders_count--;
}

internal inline f32
_world_test_edge(f32 wall_a, f32 wall_min_b, f32 wall_max_b,
           f32 entity_a, f32 entity_b,
           f32 entity_da, f32 entity_db)
{
     f32 t = (wall_a - entity_a) / entity_da;
     if (t >= 0) {
         f32 wall_b = entity_b + (t * entity_db);
         if (wall_b >= wall_min_b && wall_b <= wall_max_b) {
            return MAX(0., t-0.001);
         }
    }
    return 1.;
}

internal inline void
_world_test_collider(Collider *A, Collider *B, Collision *C)
{
    f32 half_w = (B->w + A->w);
    f32 half_h = (B->h + A->h);

    f32 min_x = B->x - half_w;
    f32 min_y = B->y - half_h;
    f32 max_x = B->x + half_w;
    f32 max_y = B->y + half_h;

    f32 t = 0.;

    if (C->delta_x != 0)
    {
        // Left edge
        if (B->edges & DirectionFlag_Left) {
            t = _world_test_edge(min_x, min_y, max_y,
                                 C->end_x, C->end_y, C->delta_x, C->delta_y);
            if (t < C->t) { C->t = t; C->normal_x = -1; C->normal_y = 0; C->B = B; }
        }

        // Right edge
        if (B->edges & DirectionFlag_Right) {
            t = _world_test_edge(max_x, min_y, max_y,
                                 C->end_x, C->end_y, C->delta_x, C->delta_y);
            if (t < C->t) { C->t = t; C->normal_x = 1; C->normal_y = 0; C->B = B; }
        }
    }

    if (C->delta_y != 0)
    {
        // Top edge
        if (B->edges & DirectionFlag_Top) {
            t = _world_test_edge(min_y, min_x, max_x,
                                 C->end_y, C->end_x, C->delta_y, C->delta_x);
            if (t < C->t) { C->t = t; C->normal_x = 0; C->normal_y = -1; C->B = B; }
        }

        // Bottom edge
        if (B->edges & DirectionFlag_Bottom) {
            t = _world_test_edge(max_y, min_x, max_x,
                                 C->end_y, C->end_x, C->delta_y, C->delta_x);
            if (t < C->t) { C->t = t; C->normal_x = 0; C->normal_y = 1; C->B = B; }
        }
    }

}

Collision *
world_test_move(World *W, Collider *A, f32 dx, f32 dy, u8 mask, Collision *C, OnCollision *on_collision)
{
#if _DEBUG
    static boolean entered = 0;
    ASSERT(entered == 0);
    entered = 1;
#endif

    ASSERT(!equalf(dx, 0, 0.1) || !equalf(dy, 0, 0.1));
    ASSERT(mask != 0);

    *C = (Collision){0};
//    C->end_x = C->begin_x = A->x;
//    C->end_y = C->begin_y = A->y;
    C->end_x = A->x;
    C->end_y = A->y;
    C->delta_x = dx;
    C->delta_y = dy;
    C->A = A;


#if 0
    V4f A__n = {{
        A->x + dx,
        A->y + dy,
        A->w,
        A->h,
    }};

    V4i cell_range = world_rect_to_cell(W, A->x, A->y, A->w, A->h, dx, dy, WORLD_CELL_SIZE);
    V4i new_cell = world_rect_to_cell(W, A->x + dx, A->y + dy, A->w, A->h, WORLD_CELL_SIZE);
    V4i cell_range = {{
        MIN(W->cell_rect.max_x, MAX(0, MIN(old_cell.min_x, new_cell.min_x))),
        MIN(W->cell_rect.max_x, MAX(0, MIN(old_cell.min_y, new_cell.min_y))),
        MIN(W->cell_rect.max_x, MAX(0, MAX(old_cell.max_x, new_cell.max_x))),
        MIN(W->cell_rect.max_y, MAX(0, MAX(old_cell.max_y, new_cell.max_y)))
    }};

    old_cell = world_rect_to_cell(W, A->x, A->y, A->w, A->h, WORLD_CELL_SIZE);
    new_cell = world_rect_to_cell(W, A->x, A->y, A->w, A->h, WORLD_CELL_SIZE);
    V4i tile_range; = {{
            MIN(W->tile_map->w, MAX(0, ))
        world_cell_to_tile(cell_range.min_x),
        world_cell_to_tile(cell_range.min_y),
        world_cell_to_tile(cell_range.max_x),
        world_cell_to_tile(cell_range.max_y)
    }};
#endif

    // TODO: Extend the range by half of the greater side of C?
    V4i cell_range = world_cell_range(A->x, A->y, A->w, A->h, dx, dy, WORLD_CELL_SIZE);
    world_cell_clip(&cell_range, &W->cell_rect);

    V4i tile_range = world_cell_range(A->x, A->y, A->w, A->h, dx, dy, WORLD_TILE_SIZE);
    world_cell_clip(&tile_range, &W->tile_rect);

    u16 *tile_data = tile_map_data(W->tile_map);
    Collider tile_collider;
    tile_collider.flags = ColliderFlag_TileMap;
    tile_collider.w = WORLD_TILE_SIZE >> 1;
    tile_collider.h = WORLD_TILE_SIZE >> 1;
    tile_collider.ptr = NULL;

    i32 i;
    Collider *B;
    Collider **ingored = C->_ignored;

    u8 retry = WORLD_COLLISION_RETRY;

    OnCollisionResult result;
    i32 cx, cy;
    WorldCell *cell;
    while (retry)
    {
        C->t = 1.;
        C->B = 0;

        // printf("from [%.3f %.3f] delta [%.3f %.3f]\n", C->end_x, C->end_y, C->delta_x, C->delta_y);
        C->over_x = C->end_x + C->delta_x;
        C->over_y = C->end_y + C->delta_y;

        // Test tile map.

        if (mask & CollisionTestMask_TileMap)
        {
            for (cy = tile_range.min_y; cy != tile_range.max_y; ++cy) {
                for (cx = tile_range.min_x; cx != tile_range.max_x; ++cx) {
                    tile_collider.edges = tile_data[cx + cy * W->tile_map->w];
                    if (tile_collider.edges) {
                        tile_collider.x = (cx * WORLD_TILE_SIZE) + (WORLD_TILE_SIZE / 2);
                        tile_collider.y = (cy * WORLD_TILE_SIZE) + (WORLD_TILE_SIZE / 2);
                        _world_test_collider(A, &tile_collider, C);
                    }
                }
            }
        }

        // Test colliders.

        if (mask & CollisionTestMask_Colliders)
        {
            for (cy = cell_range.min_y; cy != cell_range.max_y; ++cy) {
                for (cx = cell_range.min_x; cx != cell_range.max_x; ++cx) {
                    cell = W->buckets[world_bucket(world_hash(cx, cy))];
                    while (cell && (cell->x != cx && cell->y != cy)) cell = cell->next;

                    if (cell) {
                        for (i = 0; i < cell->colliders_count; ++i) {
                            B = cell->colliders[i];
                            if ((B->flags & ColliderFlag_LocalDisable) && A != B && on_collision(A, B, 0)) {
                                _world_test_collider(A, B, C);
                            }
                        } // for i
                    } // if cell
                } // for cx
            } // for cy
        }

        if (C->B)
        {
            C->delta_x *= C->t;
            C->delta_y *= C->t;

            // Move to the collision.
            C->end_x += C->delta_x;
            C->end_y += C->delta_y;
//            C->begin_x = C->end_x;
//            C->begin_y = C->end_y;

            // Get the new delta applied after the collision.
            result = on_collision(A, C->B, C);

            // If delta is 0, then there's not need to check for
            // further collisions and we're done here.
            if (equalf(C->delta_x, 0.0f, WORLD_COLLISION_EPSILON) &&
                equalf(C->delta_y, 0.0f, WORLD_COLLISION_EPSILON))
            {
                break;
            }

//            C->end_x = C->begin_x + C->delta_x;
//            C->end_y = C->begin_y + C->delta_y;

            switch (result)
            {
            case OnCollisionResult_Continue:
                break;
            case OnCollisionResult_Stop:
                goto stop;
            case OnCollisionResult_Ignore:
                // TODO: If this fails too often, implement an
                // intrusive linked-list in the Collider.
                ASSERT((ingored - C->_ignored) != WORLD_COLLISION_IGNORE);
                *ingored++ = C->B;
                C->B->flags |= ColliderFlag_LocalDisable;
                retry = WORLD_COLLISION_RETRY;
                break;
            }

        }
        else
        {
            C->end_x += C->delta_x;
            C->end_y += C->delta_y;
            break;
        }

        retry--;

    } // while retry

stop:

    if (C->B) {
        // printf("collision [%.3f %.3f]\n", C->end_x, C->end_y);
    }

    i = ingored - C->_ignored;
    while (i--) {
        (*ingored)->flags &= ~ColliderFlag_LocalDisable;
        ingored--;
    }

#if _DEBUG
    entered = 0;
#endif
    return C;
}

void
world_move(World *W, Collider *A, f32 x, f32 y)
{
    A->x = x;
    A->y = y;
    _world_cell_update(W, A);
}

#ifdef ASSETS
boolean on_collision(Collider *A, Collider *B, Collision *C) {
    unused(A);
    unused(B);
    unused(C);
    return 0;
}
#endif
