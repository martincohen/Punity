#include "core/core.h"
#include "world.h"

/*
 * TODO:
 *
 * - Remove empty cells from buckets.
 *   - Pool of empty cells?
 *   - Move last, and recalculate the buckets?
 *
 */

V4i
world_rect_to_cell(World *W, V4f *c)
{
    V4i result;

    result.min_x =  (i32)(c->x - c->w) >> WORLD_CELL_SIZE_SHIFT;
    result.min_y =  (i32)(c->y - c->h) >> WORLD_CELL_SIZE_SHIFT;
    result.max_x = ((i32)(c->x + c->w - 1) >> WORLD_CELL_SIZE_SHIFT) + 1;
    result.max_y = ((i32)(c->y + c->h - 1) >> WORLD_CELL_SIZE_SHIFT) + 1;
//    result.max_x = (((i32)(c->x + c->w - (WORLD_CELL_SIZE >> 1)) + WORLD_CELL_SIZE_SHIFT - 1) >> WORLD_CELL_SIZE_SHIFT);
//    result.max_y = (((i32)(c->y + c->h - (WORLD_CELL_SIZE >> 1)) + WORLD_CELL_SIZE_SHIFT - 1) >> WORLD_CELL_SIZE_SHIFT);

    return result;
}

#ifdef WORLD_ARRAY
#define WORLD_CHECK_RANGE(range) \
    ASSERT_DEBUG((range)->min_x >= 0); \
    ASSERT_DEBUG((range)->min_y >= 0); \
    ASSERT_DEBUG((range)->max_x < W->w); \
    ASSERT_DEBUG((range)->max_y < W->h); \

#define WORLD_FOR_EACH_CELL(range) \
    WORLD_CHECK_RANGE(range) \
    s32 _cx, _cy; \
    WorldCell *__ROW__ = W->cells + (range)->min_x + (range)->min_y * W->w; \
    WorldCell *__CELL__; \
    for (_cy = (range)->min_y; _cy != (range)->max_y; ++_cy, __ROW__ += W->w) \
    for (_cx = (range)->min_x, __CELL__ = __ROW__; _cx != (range)->max_x; ++_cx, ++__CELL__)

internal boolean
_world_cell_remove(World *W, RectI *cell, Collider *collider)
{
    u32 i;
    WORLD_FOR_EACH_CELL(cell) {
        for (i = 0; i != __CELL__->colliders_count; i++) {
            if (__CELL__->colliders[i] == collider) {
                if (__CELL__->colliders_count) {
                    __CELL__->colliders[i] = __CELL__->colliders[__CELL__->colliders_count - 1];
                }
                __CELL__->colliders_count--;
                break;
            }
        }
    }
    return 0;
}

internal boolean
_world_cell_add(World *W, RectI *cell, Collider *collider)
{
    u32 i;
    WORLD_FOR_EACH_CELL(cell) {
        ASSERT(__CELL__->colliders_count != WORLD_CELL_ENTITIES_PER_CELL);
        __CELL__->colliders[__CELL__->colliders_count] = collider;
        __CELL__->colliders_count++;
    }
    return 0;
}

internal WorldCell *
world_cell(World *W, s32 x, s32 y)
{
    ASSERT(x < W->w);
    ASSERT(y < W->h);
    return W->cells[x + (y * W->w)];
}
#endif

#ifdef WORLD_MAP
internal WorldCell *
_world_cell(World *W, i32 x, i32 y)
{
    if (x >= W->cell_rect.min_x && x < W->cell_rect.max_x &&
        y >= W->cell_rect.min_y && y < W->cell_rect.max_y)
    {
        u32 bucket = world_bucket(world_hash(x, y));
        WorldCell *cell = W->buckets[bucket];
        while (cell && cell->x != x || cell->y != y) {
            ++cell;
        }
        return cell;
    }
    return 0;
}

internal inline WorldCell*
_world_new_cell(World *W, i32 x, i32 y, WorldCell *next)
{
    ASSERT(W->cells_count != WORLD_HASH_MAP_CELLS_MAX);
    WorldCell *cell = W->cells + W->cells_count;
    W->cells_count++;

    cell->colliders_count = 0;
    cell->x = x;
    cell->y = y;
    cell->next = next;
    return cell;
}

internal boolean
_world_cell_remove(World *W, V4i *range, Collider *collider)
{
    i32 x, y;
    u32 i;
    WorldCell **bucket;
    WorldCell *cell;
    for (y = range->min_y; y != range->max_y; ++y) {
        for (x = range->min_x; x != range->max_x; ++x) {

            bucket = W->buckets + world_bucket(world_hash(x, y));
            cell = *bucket;

            while (cell && (cell->x != x || cell->y != y)) {
                cell = cell->next;
            }

            if (cell)
            {
                do {
                    for (i = 0; i < cell->colliders_count; ++i) {
                        if (cell->colliders[i] == collider) {
                            if (cell->colliders_count != 1) {
                                cell->colliders[i] = cell->colliders[cell->colliders_count - 1];
                            }
                            cell->colliders_count--;

                            if (cell->colliders_count == 0) {
                                // TODO: Remove the cell?
                            }
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

internal boolean
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

            ASSERT(cell->colliders_count != WORLD_COLLIDERS_PER_CELL_MAX);
            cell->colliders[cell->colliders_count] = collider;
            cell->colliders_count++;
        }
    }
}

WorldCell *world_cell(World *W, i32 x, i32 y)
{
    WorldCell *cell = W->buckets[world_bucket(world_hash(x, y))];
    while (cell && (cell->x != x || cell->y != y)) {
        cell = cell->next;
    }
    return cell;
}

#endif

internal void
_world_cell_update(World *W, Collider *collider)
{
    V4i cell = world_rect_to_cell(W, &collider->extents);
    // printf("%d %d %d %d\n", cell.min_x, cell.min_y, cell.max_x, cell.max_y);

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

    collider->x = x;
    collider->y = y;
    collider->w = w * 0.5;
    collider->h = h * 0.5;
    collider->ptr = ptr;
    collider->cell = world_rect_to_cell(W, &collider->extents);
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

void
world_move(World *W, Collider *A, f32 dx, f32 dy, Collision *C)
{
    if ((i32)dx == 0 && (i32)dy == 0) {
        return;
    }

    V4f A__n = {
        A->x + dx,
        A->y + dy,
        A->w,
        A->h,
    };

    // TODO: Extend the range by half of the greater side of C?

    V4i old_cell = world_rect_to_cell(W, (V4f*)&A);
    V4i new_cell = world_rect_to_cell(W, &A__n);
    V4i range = {
        MIN(old_cell.min_x, new_cell.min_x),
        MIN(old_cell.min_y, new_cell.min_y),
        MAX(old_cell.max_x, new_cell.max_x),
        MAX(old_cell.max_y, new_cell.max_y)
    };

    f32 half_w, half_h;

    V4f r;
    f32 t;
    i32 i;
    i32 c;
    Collider *B;
    Collider **disabled = C->_disabled;

    u8 retry = WORLD_COLLISION_RETRY;
    *C = (Collision){0};
    C->dx = dx;
    C->dy = dy;

    i32 cx, cy;
    WorldCell *cell;
    while (retry)
    {
        C->t = 1.;
        C->B = 0;

        C->x = A->x + C->dx;
        C->y = A->y + C->dy;

        for (cy = range.min_y; cy != range.max_y; ++cy)
        {
            for (cx = range.min_x; cx != range.max_x; ++cx)
            {
                cell = W->buckets[world_bucket(world_hash(cx, cy))];
                while (cell && (cell->x != cx && cell->y != cy)) {
                    cell = cell->next;
                }

                if (cell)
                {
                    for (i = 0;
                         i < cell->colliders_count;
                         ++i)
                    {
                        B = cell->colliders[i];
                        if ((B->flags & ColliderFlag_LocalDisable) ||
                             A == B ||
                             !on_collision(A, B, 0)) continue;

                        half_w = (B->w + A->w);
                        half_h = (B->h + A->h);

                        r.min_x = B->x - half_w;
                        r.min_y = B->y - half_h;
                        r.max_x = B->x + half_w;
                        r.max_y = B->y + half_h;

                        c = 0;

                        if (C->dx != 0)
                        {
                            // Left edge
                            t = _world_test_edge(r.min_x, r.min_y, r.max_y,
                                                 A->x, A->y, C->dx, C->dy);
                            if (t < C->t) { C->t = t; C->nx = -1; C->ny = 0; c = 1; }

                            // Right edge
                            t = _world_test_edge(r.max_x, r.min_y, r.max_y,
                                                 A->x, A->y, C->dx, C->dy);
                            if (t < C->t) { C->t = t; C->nx = 1; C->ny = 0; c = 1; }
                        }

                        if (C->dy != 0)
                        {
                            // Top edge
                            t = _world_test_edge(r.min_y, r.min_x, r.max_x,
                                                 A->y, A->x, C->dy, C->dx);
                            if (t < C->t) { C->t = t; C->nx = 0; C->ny = -1; c = 1; }

                            // Bottom edge
                            t = _world_test_edge(r.max_y, r.min_x, r.max_x,
                                                 A->y, A->x, C->dy, C->dx);
                            if (t < C->t) { C->t = t; C->nx = 0; C->ny = 1; c = 1; }
                        }

                        if (c) {
                            C->cx = A->x + (C->t * C->dx);
                            C->cy = A->y + (C->t * C->dy);
                            C->A = A;
                            C->B = B;
                        }

                    } // if cell
                } // for cx
            } // for cy
        } // while retry

        if (C->B)
        {
            if (on_collision(A, C->B, C) == 0) {
                *disabled++ = C->B;
                C->B->flags |= ColliderFlag_LocalDisable;
            }
        }
        else
        {
            A->x += dx;
            A->y += dy;
            break;
        }

        retry--;
    }

    _world_cell_update(W, A);

    i = disabled - C->_disabled;
    while (i--) {
        (*disabled)->flags &= ~ColliderFlag_LocalDisable;
        disabled--;
    }
}

