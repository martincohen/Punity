// Build with: build example-platformer

// TODO: Player sprite and animation (walk, jump)
// TODO: Death condition.
// TODO: Camera.
// TODO: Parallax.

#define PUNITY_IMPLEMENTATION
#include "punity.h"

enum {
    SceneLayer_Ground = 1 << 0,
    SceneLayer_Player = 1 << 1,
};

#define TILEDTILE_DEFAULT_LAYER SceneLayer_Ground

#define TILED_IMPLEMENTATION
#include "punity-tiled.h"

typedef struct
{
    SceneEntity *E;
    int jump;
    int fall;
    int grounded;
}
Player;

typedef struct
{
    // Target position.
    f32 tx, ty;
    // Actual position.
    f32 x, y;
}
Camera;

typedef struct Game_
{
    Bitmap font;
    Scene scene;
    TiledScene scene1;
    Bitmap tileset;
    Sound sound_jump;
    Sound sound_land;

    Camera camera;
    Player player;
}
Game;

static Game *GAME = 0;

//
// Player
//

void
player_init(Player *P, TiledItem *item)
{
    P->E = scene_entity_add(&GAME->scene,
        item->rect,
        SceneLayer_Player,
        SceneLayer_Player | SceneLayer_Ground);
    P->jump = 0;
    P->fall = 1;
}

void
player_step(Player *P, int key_left, int key_right, int key_jump)
{
    Collision C = {0};

    // Left & right.

    int dx = 0;
    if (key_right) { dx = +1; }
    if (key_left)  { dx = -1; }
    scene_entity_move_x(&GAME->scene, P->E, dx * 2, &C);

    // Jump & fall.

    static int PLAYER_JUMP[] = { 5, 4, 3, 2, 2, 1, 1, 1 };

    if (key_jump && P->grounded) {
        P->jump = 1;
        sound_play(&GAME->sound_jump);
    }

    if (!P->jump) {
        P->fall = minimum(P->fall + 1, 16);
        if (!scene_entity_move_y(&GAME->scene, P->E, P->fall, &C)) {
            if (!P->grounded) {
                sound_play(&GAME->sound_land);
                P->grounded = 1;
            }
            P->fall = 2;
        } else {
            P->grounded = 0;
        }
    } else {
        P->grounded = 0;
        P->jump++;
        if (P->jump >= array_count(PLAYER_JUMP) ||
            scene_entity_move_y(&GAME->scene, P->E, -(PLAYER_JUMP[P->jump - 1] * 2), &C) == false)
        {
            P->jump = 0;
        }
    }

    rect_pos(P->E->box, 0.5, 0.5, &GAME->camera.tx, &GAME->camera.ty);
    GAME->camera.x += (GAME->camera.tx - GAME->camera.x) / 4.0f;
    GAME->camera.y += (GAME->camera.ty - GAME->camera.y) / 4.0f;
    camera_set(roundf(GAME->camera.x), roundf(GAME->camera.y), 0);

    // Draw the player.
    rect_draw_push(P->E->box, 2, 10);
}

//
// Scene
//

int
scene_setup(TiledScene *tiled_scene)
{    
    // EXAMPLE FOR QUICK SETUP:
    // Set this tilemap as base for collision detection.
    // GAME->scene.tilemap = &tiledscene_find(tiled_scene, "base", 0)->tilemap;
    // Set player from `player` rectangle object.
    // player_init(&GAME->player, tiledscene_find(tiled_scene, "player", 0)->item);

    // MORE ADVANCED SETUP:
    // Initialize entities.
    for (TiledItem *item = tiled_scene->items;
         item != tiled_scene->items_end;
         item = tileditem_next(item))
    {
        switch (item->type)
        {
        case TiledType_TileMap:
            if (strcmp(item->name, "base") == 0) {
                GAME->scene.tilemap = &item->tilemap;
            }
            break;
        case TiledType_Rectangle:
            if (strcmp(item->name, "player") == 0) {
                player_init(&GAME->player, item);
            }
            break;
        case TiledType_Image:
            break;
        }
    }

    return 1;
}

//
// Game
//

int
init()
{
    CORE->window.width = 16 * 8;
    CORE->window.height = 16 * 8;
    CORE->window.scale = 5;

    GAME = bank_push_t(CORE->storage, Game, 1);

    sound_load_resource(&GAME->sound_jump, "jump.ogg");
    sound_load_resource(&GAME->sound_land, "land.ogg");

    font_load_resource(&GAME->font, "font.png", 4, 7);
    CORE->canvas.font = &GAME->font;

    bitmap_load_resource(&GAME->tileset, "tileset.png", 0);
    GAME->tileset.tile_width  = 8;
    GAME->tileset.tile_height = 8;

    // Load scenes.
    Tiled tiled;
    tiled_init(&tiled);
    tiledscene_load_resource(&tiled, &GAME->scene1, "map1.json", 0, 0);
    tiled_free(&tiled);

    // Initialize game scene.
    scene_init(&GAME->scene, 16);

    // Start at first scene.
    scene_setup(&GAME->scene1);

    return 1;
}

void
step()
{
    canvas_clear(1);

    if (key_down(KEY_ESCAPE)) {
        CORE->running = 0;
    }

    if (key_pressed(KEY_RETURN)) {
        window_fullscreen_toggle();
    }

    player_step(&GAME->player,
        key_down(KEY_LEFT), key_down(KEY_RIGHT), key_pressed(KEY_UP));

    tilemap_draw(GAME->scene.tilemap);
 
    camera_reset();   
    char buf[256];
    sprintf(buf, "%.3fms %.3f", CORE->perf_step * 1e3, CORE->time_delta);
    text_draw(buf, 0, 0, 2);
}