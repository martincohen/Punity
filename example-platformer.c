// Build with: build example-platformer

// TODO: Player sprite and animation (walk, jump)
// TODO: Death condition.
// TODO: Camera.
// TODO: Parallax.

#define PUNITY_IMPLEMENTATION
#include "punity.h"

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

typedef struct Game_
{
    Bitmap font;
    Scene scene;
    Bitmap tileset;
    Sound sound_jump;
    Sound sound_land;

    Player player1;
    Player player2;
}
Game;

static Game *GAME = 0;

enum {
    SceneLayer_Ground = 1 << 0,
    SceneLayer_Player = 1 << 1,
};

//
// Player
//

void
player_init(Player *P, int x, int y)
{
    P->E = scene_entity_add(&GAME->scene,
        rect_make_size(x, y, 8, 8),
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

    // Draw the player.
    rect_draw_push(P->E->box, 2, 10);
}

//
// Game
//

#define _ 0
static int tiles[] = {
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _, 89,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  9, 10, 11, 12, 13,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  7,  _,  6,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  1,  3,  2,  4,  3,  5,  _, 22, 23,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 30, 31,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  1,  4,  3,  5,  _,  _,  _,
        _,  _,  _,  _, 88,  _,  _, 15,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  1,  2,  3,  4,  2,  3,  4,  2,  3,  5,  _,  _,  _, 
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
};
#undef _

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

    scene_init(&GAME->scene, 8, 16, 16, 16);
    
    int *t = tiles;
    for (int y = 0; y != 16; ++y) {
        for (int x = 0; x != 16; ++x, ++t) {
            SceneTile *tile = scene_tile(&GAME->scene, x, y);
            switch (*t)
            {
            // Empty tile and decorations.
            case  0:
            case  6: case  7: case 14: case 15:
            case 22: case 23: case 30: case 31:
                break;
            case 88:
                player_init(&GAME->player1, x * 8, y * 8);
                break;
            
            case 89:
                player_init(&GAME->player2, x * 8, y * 8);
                break;

            default:
                tile->edges = Edge_Top;
                tile->layer = SceneLayer_Ground;
                break;
            }
        }
    }



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

    player_step(&GAME->player1,
        key_down(KEY_LEFT), key_down(KEY_RIGHT), key_pressed(KEY_UP));

    player_step(&GAME->player2,
        key_down(KEY_A), key_down(KEY_D), key_pressed(KEY_W));

    // Draw tilemap.
    int *t = tiles;
    for (int y = 0; y != 16; ++y) {
        for (int x = 0; x != 16; ++x, ++t) {
            switch (*t) {
            case 0: case 88: case 89: break;
            default:
                tile_draw_push(&GAME->tileset, x * 8, y * 8, *t, 4);
            }
        }
    }
    
    char buf[256];
    sprintf(buf, "%.3fms %.3f", CORE->perf_step * 1e3, CORE->time_delta);
    text_draw(buf, 0, 0, 2);
}