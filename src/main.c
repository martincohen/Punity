#include "core/core.h"
#include "main.h"

#ifdef ASSETS

void
assets(AssetWrite *out)
{
    assets_add_image_palette(out, "palette.png");
    assets_add_image_set(out, "font.png", 4, 7, 0, 0, ((V4i){1, 0, 0, 0}));
    assets_add_image_set(out, "icons.png", 7, 7, 0, 0, ((V4i){0}));
    assets_add_image_set(out, "player_walk.png", 8, 8, 4, 8, ((V4i){0}));
    assets_add_image_set(out, "player_idle.png", 8, 8, 4, 8, ((V4i){0}));
    assets_add_image_set(out, "player_brake.png", 8, 8, 4, 8, ((V4i){0}));
    assets_add_image_set(out, "tileset.png", 8, 8, 0, 0, ((V4i){0}));
    assets_add_tile_map(out, "map.csv");
}

#else

#include "world.c"
#include "debug.c"

Animation player_walk = {
    .begin = 0,
    .end = 4,
    .sprite = 0,
    .set = _asset_player_walk
};

Animation player_idle = {
    .begin = 0,
    .end = 1,
    .sprite = 0,
    .set = _asset_player_idle
};

Animation player_brake = {
    .begin = 0,
    .end = 2,
    .sprite = 0,
    .set = _asset_player_brake
};

Entity *
entity_add(i32 x, i32 y, i32 w, i32 h)
{
    Entity *entity = static_array_push(STATE->entities, 1);
    *entity = (Entity){0};
    entity->animation = static_array_push(STATE->animations, 1);
    *entity->animation = (Animation){0};
    entity->collider = world_add(&STATE->world, x, y, w, h, entity);

    return entity;
}

void
init()
{
    memcpy(PROGRAM->palette, &asset_palette->colors, asset_palette->colors_count * sizeof(Color));
    PROGRAM->font = asset_font;

    STATE->player = entity_add(32, 16 - 4, 8, 8);
    STATE->player->sprite_ox = 0;
    STATE->player->sprite_oy = 4;
    STATE->player->dx = 1;
    *STATE->player->animation = player_idle;
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
    r.max_x = MIN(tile_map->w, ceil_div(r.max_x, set->cw));
    r.max_y = MIN(tile_map->h, ceil_div(r.max_y, set->ch));

    u16 *tile = asset_data(tile_map);
    for (i32 y = r.min_y; y != r.max_y; y++) {
        for (i32 x = r.min_x; x != r.max_x; x++) {
            if (tile[x] != 0xFFFF)
            {
                set_draw(x * set->cw,
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

    if (button_down(Button_Right)) dx = 1;
    if (button_down(Button_Left))  dx = -1;
    if (button_ended_down(Button_X)) {
        P->vy = -1;
    }

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

boolean
on_collision(Collider *A, Collider *B, Collision *collision)
{
    if (B < A) SWAP(Collider*, A, B);

    if (collision) {
    } else {
    }

    return 1;
}

static Collision collision;

void
step()
{
    draw_set(COLOR_BLACK);

    Entity *entity;
    memi i;

    player_step(STATE->player);

    //
    // Update entities
    //

    entity = STATE->entities;
    for (i = 0;
         i != STATE->entities_count;
         ++i, ++entity)
    {
        world_move(&STATE->world, entity->collider, entity->vx, entity->vy, &collision);

//        if (entity->fx > entity->vx) {
//            entity->vx += MAX(1, (entity->fx - entity->vx) >> 1);
//        } else if (entity->fx < entity->vx) {
//            entity->vx -= MAX(1, (entity->vx - entity->fx) >> 1);
//        }
    }

    player_camera_step(STATE->player, &STATE->camera);

    PROGRAM->tx = -STATE->camera.x + (SCREEN_WIDTH >> 1);
    PROGRAM->ty = -STATE->camera.y + (SCREEN_HEIGHT >> 1);

    //
    // Update animations
    //

    if ((PROGRAM->frame % 3) == 0)
    {
        Animation *ani = STATE->animations;
        for (i = 0;
             i != STATE->animations_count;
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

    tilemap_draw(asset_map, asset_tileset, 0, 0);

    Collider *collider;
    entity = STATE->entities;
    for (i = 0;
         i != STATE->entities_count;
         ++i, ++entity)
    {
        collider = entity->collider;
//        rect_draw(recti_make_size(collider->x - collider->w,
//                                  collider->y - collider->h,
//                                  collider->w * 2,
//                                  collider->h * 2), COLOR_SHADE_3);
        set_draw(collider->x + entity->sprite_ox, collider->y + entity->sprite_oy,
                 entity->animation->set,
                 entity->animation->sprite,
                 entity->animation->sprite_mode, 0);
    }

    //
    // Debug
    //

    debug_world_colliders_count(&STATE->world);

    PROGRAM->tx = 0;
    PROGRAM->ty = 0;
    debug_buttons(4, SCREEN_HEIGHT - asset_icons->ch - 4);
    debug_fps(SCREEN_WIDTH - (7 * asset_font->cw) - 4,
              SCREEN_HEIGHT - asset_font->ch - 4,
              PROGRAM->time_step);
}

#endif
