#include "src/id_enum.h"
#include "src/res.h"

#include "src/main.h"

#include "lib/punity/punity_world.c"
#include "src/debug.c"
#include "src/player.c"

ProgramState *GAME = 0;

// TODO: The `asset_*` functions should return pointer to the asset to be set correctly.
// TODO: For map, actually create the entities and the world and store it to the resources.

// TODO: STATE should be static/malloc variable (depending on what client wants).
//       That would simplify how it's all bound together.
//       punity_state(size) should do the allocation in DEBUG though so that the STATE
//       can be kept between the reloads.

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

void
camera_constrain(Camera *camera)
{
    camera->x = MAX(camera->x, (SCREEN_WIDTH / 2));
    camera->y = MAX(camera->y, (SCREEN_HEIGHT / 2));
    camera->x = MIN(camera->x, (i32)(GAME->world.tile_map->w * WORLD_TILE_SIZE) - (SCREEN_WIDTH / 2));
    camera->y = MIN(camera->y, (i32)(GAME->world.tile_map->h * WORLD_TILE_SIZE) - (SCREEN_HEIGHT / 2));
}

//
// Callbacks
//

void
load()
{
    GAME = PROGRAM->state;

    GAME->level = res_level_1;

    PROGRAM->asset_files = res_files;
    memcpy(PROGRAM->palette, &res_palette->colors, res_palette->colors_count * sizeof(Color));
    PROGRAM->font = res_font;

    world_load(&GAME->world, ASSET(TileMap, GAME->level->map_collision));
}


void
init()
{
    GAME->player = player_init();
    sound_play(res_sound_music, AudioSourceFlag_Loop);
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
    if (button_down(&PROGRAM->input, Button_Escape)) {
        PROGRAM->running = 0;
    }

    // printf("\nSTEP %d\n", PROGRAM->frame);
    draw_set(COLOR_SHADE_3);

    Entity *entity;
    memi layer_i;

    player_step(&PROGRAM->input, GAME->player);

    //
    // Update entities
    //

    entity = GAME->entities;
    for (layer_i = 0;
         layer_i != GAME->entities_count;
         ++layer_i, ++entity)
    {
        if (!equalf(entity->vx, 0.0f, WORLD_COLLISION_EPSILON) ||
            !equalf(entity->vy, 0.0f, WORLD_COLLISION_EPSILON))
        {
            world_test_move(&GAME->world,
                            entity->collider,
                            entity->vx, entity->vy,
                            CollisionTestMask_All,
                            &collision,
                            on_collision);

            if ((!equalf(collision.end_x, entity->collider->x, WORLD_COLLISION_EPSILON) ||
                 !equalf(collision.end_y, entity->collider->y, WORLD_COLLISION_EPSILON)))
            {
                world_move(&GAME->world,
                           entity->collider,
                           collision.end_x,
                           collision.end_y);

//                entity->collider->x = roundf(entity->collider->x);
//                entity->collider->y = roundf(entity->collider->y);
            }
        }
    }

    player_camera_step(GAME->player, &GAME->camera);
    camera_constrain(&GAME->camera);

    PROGRAM->tx = -GAME->camera.x + (SCREEN_WIDTH >> 1);
    PROGRAM->ty = -GAME->camera.y + (SCREEN_HEIGHT >> 1);

    //
    // Update animations
    //

    if ((PROGRAM->frame % 3) == 0)
    {
        Animation *ani = GAME->animations;
        for (layer_i = 0;
             layer_i != GAME->animations_count;
             ++layer_i, ++ani)
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

    // TODO: Move level drawing to a separate function.
    //     - Also keep the current level data, update them in load().

    Collider *collider;

    layer_i = 0;

    // Background layers
    LayerProperties *layer_properties = GAME->level->layer_properties;
    AssetHandle *layer = GAME->level->layers;
    for (; layer_i <= GAME->level->layer_main; ++layer_properties, ++layer, ++layer_i) {
        if (layer_properties->parallax == 0) {
            tile_map_draw(ASSET(TileMap, *layer),
                          res_tileset, 0, 0);
        } else if (layer_properties->parallax > 0) {
            tile_map_draw(ASSET(TileMap, *layer),
                          res_tileset,
                          -(PROGRAM->tx/layer_properties->parallax),
                          -(PROGRAM->ty/layer_properties->parallax));
        } else if (layer_properties->parallax < 0) {
            tile_map_draw(ASSET(TileMap, *layer),
                          res_tileset,
                          -(PROGRAM->tx*-layer_properties->parallax),
                          -(PROGRAM->ty*-layer_properties->parallax));
        }
    }

    // Draw entities.
    entity = GAME->entities;
    for (u32 i = 0; i != GAME->entities_count; ++i, ++entity)
    {
        collider = entity->collider;
//                rect_draw(v4i_make_size(roundf(collider->x - collider->w),
//                                        roundf(collider->y - collider->h),
//                                        collider->w * 2,
//                                        collider->h * 2), COLOR_SHADE_3);
        image_set_draw(roundf(collider->x + entity->sprite_ox),
                       roundf(collider->y + entity->sprite_oy),
                       ASSET(ImageSet, entity->animation->set),
                       entity->animation->sprite,
                       entity->animation->sprite_mode, 0);
    }

    // Foreground layers
    for (; layer_i != GAME->level->layers_count; ++layer_i) {
        tile_map_draw(ASSET(TileMap, GAME->level->layers[layer_i]), res_tileset, 0, 0);
    }

    //
    // Debug
    //

#ifdef _DEBUG
    // debug_world_colliders_count(&GAME->world);
#endif

    PROGRAM->tx = 0;
    PROGRAM->ty = 0;
    debug_fps(SCREEN_WIDTH - (7 * res_font->cw) - 4,
              SCREEN_HEIGHT - res_font->ch - 4,
              PROGRAM->time_step);
#ifdef _DEBUG
    debug_buttons(&PROGRAM->input, 4, SCREEN_HEIGHT - res_icons->ch - 4);
    // debug_world_bucket_stats(&GAME->world, 4, 4);
    // debug_world_bucket_cells(&GAME->world, SCREEN_WIDTH - 4 - 16, 4);
#endif
}
