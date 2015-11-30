#include "punity_core.h"
#include "main.h"
#include "res/res.h"
#include <math.h>

#include "debug.c"

ProgramState *GAME = 0;

// TODO: The `asset_*` functions should return pointer to the asset to be set correctly.
// TODO: For map, actually create the entities and the world and store it to the resources.

// TODO: STATE should be static/malloc variable (depending on what client wants).
//       That would simplify how it's all bound together.
//       punity_state(size) should do the allocation in DEBUG though so that the STATE
//       can be kept between the reloads.



Animation player_walk = {
    .begin = 0,
    .end = 4,
    .sprite = 0,
    .set = res_player_walk
};

Animation player_idle = {
    .begin = 0,
    .end = 1,
    .sprite = 0,
    .set = res_player_idle
};

Animation player_brake = {
    .begin = 0,
    .end = 2,
    .sprite = 0,
    .set = res_player_brake
};

//Level level_1 = {
//    .layers = { res_level_0_layer1 },
//    .layers_count = 1,
//    .map_collision = res_level_0_collision,
//    .map_meta = 0,
//    .set_meta = 0,
//    .set_main = res_image_set_main,

//};

//void
//set_level(Level *level)
//{
//    GAME->tile_map = leve
//}

//
// Entities
//

Entity *
entity_add(i32 x, i32 y, i32 w, i32 h, i32 ox, i32 oy)
{
    f32 fw = w * 0.5;
    f32 fh = h * 0.5;
    f32 fx = x + fw + (f32)ox;
    f32 fy = y + fh - (f32)oy;


    Entity *entity = static_array_push(GAME->entities, 1);
    *entity = (Entity){0};
    entity->sprite_ox = ox;
    entity->sprite_oy = oy;
    entity->animation = static_array_push(GAME->animations, 1);
    *entity->animation = (Animation){0};
    entity->collider = world_add(&GAME->world, fx, fy, fw, fh, entity);

    return entity;
}

//
// Callbacks
//

void
init()
{
    memcpy(PROGRAM->palette, &res_palette->colors, res_palette->colors_count * sizeof(Color));
    PROGRAM->font = res_font;
    GAME = PROGRAM->state;

    GAME->world.tile_map = res_level_0_collision;
    GAME->world.tile_rect.max_x = res_level_0_collision->w;
    GAME->world.tile_rect.max_y = res_level_0_collision->h;

    // GAME->player = entity_add(31, 15 - 4, 4, 8);
    GAME->player = entity_add(3 * 8 + 2, 0 * 8 + 2, 4, 8, 0, 4);
    GAME->player->dx = 1;
    *GAME->player->animation = player_idle;
}

void
tilemap_draw(TileMap *tile_map, ImageSet *set, i32 dx, i32 dy)
{
    unused(dx);
    unused(dy);

    V4i r = PROGRAM->screen_rect;
    rect_tr(&r, -PROGRAM->tx, -PROGRAM->ty);
    r.min_x = MAX(0, ((r.min_x) / (i32)set->cw));
    r.min_y = MAX(0, ((r.min_y) / (i32)set->ch));
    r.max_x = MIN((i32)tile_map->w, ceil_div(r.max_x, set->cw));
    r.max_y = MIN((i32)tile_map->h, ceil_div(r.max_y, set->ch));

    u16 *tile = tile_map_data(tile_map);
    for (i32 y = r.min_y; y != r.max_y; y++) {
        for (i32 x = r.min_x; x != r.max_x; x++) {
            if (tile[x] != 0xFFFF)
            {
                image_set_draw(x * set->cw,
                         y * set->ch,
                         set,
                         tile[x],
                         0, 0);
            }
        }
        tile += tile_map->w;
    }
}

internal void
player_set_state(Entity *P, u8 state)
{
    switch (state)
    {
    case EntityState_Idle:
        *P->animation = player_idle;
        break;
    case EntityState_Brake:
        *P->animation = player_brake;
        break;
    case EntityState_Walk:
        *P->animation = player_walk;
        break;
    }
    P->state = state;
    P->animation->sprite = 0;
}

#define signof(v) ((v > 0) - (v < 0))

internal void
player_step(Entity *P)
{
    i32 dx = 0;

    Collision C;
    world_test_move(&GAME->world, P->collider, 0, 1, CollisionTestMask_TileMap, &C, collision_stop);
    P->flags = C.B ? (P->flags | EntityFlag_Grounded) : (P->flags & ~EntityFlag_Grounded);

    if (button_down(Button_Right)) dx = 1;
    if (button_down(Button_Left))  dx = -1;

    if (P->flags & EntityFlag_Grounded) {
        // printf("Grounded");
        P->vy = 0;
        if (button_ended_down(Button_X)) {
            P->vy = -3;
        }
    }
    else
    {
        P->vy += 0.4;
    }

    // printf("vy %f\n", P->vy);

    i32 fx = 0;
    if (signof(P->vx) != dx)
    {
        if (dx == 0) {
            player_set_state(P, EntityState_Brake);
        } else {
            entity_flip(P, (dx < 0));
            player_set_state(P, EntityState_Walk);
            P->dx = dx;
        }
    }

    fx = dx * 2;

    if (fx > P->vx) {
        P->vx++;
    } else if (fx < P->vx) {
        P->vx--;
    }

    if (dx == 0 && P->vx == 0 && P->state == EntityState_Brake) {
        player_set_state(P, EntityState_Idle);
    }
}

#if 0
s32 speeds[] = { 0, 1, 2, 2, 3, 3, 3, 4, 6 };

internal void
player_camera_step(Entity *P, Camera *camera)
{
    s32 dx = (P->x + P->dx * 16) - camera->x;
    s32 sign = signof(dx);

    if (abs(dx) > 16) {
        dx = sign * 16;
    }
    camera->vx = sign * (speeds[abs(dx) / 2] * 2);
    camera->x += camera->vx;
    camera->y = P->y + 32;
}
#endif

internal void
player_camera_step(Entity *P, Camera *camera)
{
    i32 distance = 24;
    i32 dx;
//    if (P->vx) {
        dx = (entity_center_x(P) + P->dx * distance) - camera->x;
//    } else {
//        dx = P->x - camera->x;
//    }

    if (abs(dx) > 16) {
        if (P->vx) {
            camera->vx = signof(P->vx) * (abs(P->vx)) + (dx / 7);
        } else {
            camera->vx = signof(P->vx);
        }
    } else {
        if (P->vx) {
            camera->vx = P->vx + (dx / 5);
        } else {
            camera->vx = dx / 5;
        }
    }

    camera->x += camera->vx;
    camera->y = P->collider->y + 32;
}

static Collision collision;

WORLD_ON_COLLISION(on_collision)
{
    if (C == 0) {
        return 1;
    }
    // if (B < A) SWAP(Collider*, A, B);

    if ((A == GAME->player->collider) && (B->flags & ColliderFlag_TileMap))
    {
        return collision_glide(A, B, C);
    }
    return collision_stop(A, B, C);
}

void
step()
{
    // printf("\nSTEP %d\n", PROGRAM->frame);
    draw_set(COLOR_BLACK);

    Entity *entity;
    memi i;

    player_step(GAME->player);

    //
    // Update entities
    //

    entity = GAME->entities;
    for (i = 0;
         i != GAME->entities_count;
         ++i, ++entity)
    {
        if (!equalf(entity->vx, 0, 0.1) || !equalf(entity->vy, 0, WORLD_COLLISION_EPSILON))
        {
            world_test_move(&GAME->world,
                            entity->collider,
                            entity->vx, entity->vy,
                            CollisionTestMask_All,
                            &collision,
                            on_collision);

            if (!equalf(collision.end_x, entity->collider->x, WORLD_COLLISION_EPSILON) ||
                !equalf(collision.end_y, entity->collider->y, WORLD_COLLISION_EPSILON))
            {
                world_move(&GAME->world,
                           entity->collider,
                           collision.end_x,
                           collision.end_y);
            }
        }
    }

    player_camera_step(GAME->player, &GAME->camera);

    PROGRAM->tx = -GAME->camera.x + (SCREEN_WIDTH >> 1);
    PROGRAM->ty = -GAME->camera.y + (SCREEN_HEIGHT >> 1);

    //
    // Update animations
    //

    if ((PROGRAM->frame % 3) == 0)
    {
        Animation *ani = GAME->animations;
        for (i = 0;
             i != GAME->animations_count;
             ++i, ++ani)
        {
            ani->sprite++;
            if (ani->sprite == ani->end) {
                ani->sprite = ani->begin;
            }
        }
    }

    //
    // Draw
    //

    tilemap_draw(res_level_0_layer1, res_image_set_main, 0, 0);

    Collider *collider;
    entity = GAME->entities;
    for (i = 0;
         i != GAME->entities_count;
         ++i, ++entity)
    {
        collider = entity->collider;
        rect_draw(v4i_make_size(roundf(collider->x - collider->w),
                                roundf(collider->y - collider->h),
                                collider->w * 2,
                                collider->h * 2), COLOR_SHADE_3);
//        image_set_draw(collider->x + entity->sprite_ox, collider->y + entity->sprite_oy,
//                 entity->animation->set,
//                 entity->animation->sprite,
//                 entity->animation->sprite_mode, 0);
    }

    //
    // Debug
    //

    debug_world_colliders_count(&GAME->world);

    PROGRAM->tx = 0;
    PROGRAM->ty = 0;
    debug_buttons(4, SCREEN_HEIGHT - res_icons->ch - 4);
    debug_fps(SCREEN_WIDTH - (7 * res_font->cw) - 4,
              SCREEN_HEIGHT - res_font->ch - 4,
              PROGRAM->time_step);
    debug_world_bucket_stats(&GAME->world, 4, 4);
    debug_world_bucket_cells(&GAME->world, SCREEN_WIDTH - 4 - 16, 4);
}
