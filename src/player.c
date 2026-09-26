#include "lib/punity/punity_engine.h"
#include "src/main.h"
#include "src/res.h"

#include "src/id_enum.h"

#if 0
internal void
player_state_set(Entity *P, u8 state)
{
    switch (state)
    {
    case EntityState_Idle:
        P->animation->set = res_player_idle;
        P->animation->end = res_player_idle->count;
        P->animation->sprite = 0;
        break;
    case EntityState_Brake:
        P->animation->set = res_player_brake;
        P->animation->end = res_player_brake->count;
        P->animation->sprite = 0;
        break;
    case EntityState_Walk:
        P->animation->set = res_player_walk;
        P->animation->end = res_player_walk->count;
        P->animation->sprite = 0;
        break;
    }
}
#endif


Entity *
player_init()
{
    Entity *P = entity_add(3 * 8 + 2, 0 * 8 + 2, 4, 8, 0, 4);

    P->dx = 1;
    P->state = ID_Init;
    // player_state_set(P, EntityState_Idle);

    return P;
}

static Collision _collision;

#if 0
internal boolean
event_test(ProgramInput *input, Entity *P, u32 event)
{
    switch (event)
    {
    case ID_InputRightEndedDown:
        return button_ended_down(input, Button_Right);
    case ID_InputRightDown:
        return button_down(input, Button_Right);
    case ID_InputRightUp:
        return button_up(input, Button_Right);

    case ID_InputLeftEndedDown:
        return button_ended_down(input, Button_Left);
    case ID_InputLeftDown:
        return button_down(input, Button_Left);
    case ID_InputLeftUp:
        return button_up(input, Button_Left);

    case ID_InputJumpDown:
        return button_down(input, Button_B);
    case ID_InputJumpUp:
        return button_down(input, Button_B);
    case ID_InputJumpEndedDown:
        return button_down(input, Button_B);

    case ID_EntityCeil:
        world_test_move(&GAME->world, P->collider, 0, -0.01f, CollisionTestMask_TileMap, &C, collision_stop);
        return (boolean)C.B;
    case ID_EntityFloor:
        world_test_move(&GAME->world, P->collider, 0, 0.01f, CollisionTestMask_TileMap, &C, collision_stop);
        return (boolean)C.B;
    }

    return 0;
}
#endif

internal i32
is_floor(Entity *E)
{
    world_test_move(&GAME->world, E->collider, 0, 0.05f, CollisionTestMask_TileMap, &_collision, collision_stop);
    return (i32)(_collision.B && _collision.normal_y < 0);
}

internal i32
is_ceil(Entity *E)
{
    world_test_move(&GAME->world, E->collider, 0, -0.05f, CollisionTestMask_TileMap, &_collision, collision_stop);
    return (i32)(_collision.B && _collision.normal_y > 0);
}

internal void
player_step(ProgramInput *input, Entity *P)
{
    u32 state_last = P->state;

    i32 dx = 0;
    if (button_down(input, Button_Left)) {
        dx = -1;
    }
    if (button_down(input, Button_Right)) {
        dx = 1;
    }

    if (dx) {
        P->dx = dx;
        entity_flip(P, P->dx == -1);
    }

    u32 state_previous;
    do {
        state_previous = P->state;
        switch (P->state)
        {
        case ID_Idle: {
            if (dx) {
                P->state = ID_Walk;
            } else {
                P->vx = 0;
                P->vy = 0;
                if (button_down(input, Button_B)) {
                    P->state = ID_Jump;
                }
            }
        } break;

        case ID_Walk: {
            if (!is_floor(P)) {
                P->state = ID_Fall;
            } else {
                if ((PROGRAM->frame % 4) == 0) {
                    sound_play(res_sound_step, 0);
                }
                P->vx = P->dx * 2;
                if (!dx) {
                    P->state = ID_Idle;
                } else if (button_down(input, Button_B)) {
                    P->state = ID_Jump;
                }
            }
        } break;

        case ID_Jump: {
            sound_play(res_sound_jump, 0);
            P->vy = -9;
            P->state = ID_Fall;
        } break;

        case ID_Fall: {
            if ((P->vy > 0 && is_floor(P))) {
                P->state = ID_Idle;
            } else if (P->vy < 0 && is_ceil(P)) {
                P->vy = 0;
            } else {
                P->vy = MIN(5, P->vy + 1);
                P->vx = dx * 2;
            }
        } break;

        case ID_Init: {
            P->state = ID_Fall;
        } break;
        }

        if (P->state != state_previous) {
            // printf("v--------------- transition %s\n", IDNames[P->state]);
        } else {
            // printf("STATE %s\n", IDNames[P->state]);
        }
        // printf("vy %.3f\n", P->vy);

    } while (P->state != state_previous);

    if (P->state != state_last)
    {
        P->animation->begin = 0;
        switch (P->state)
        {
        default:
        case ID_Idle:
            P->animation->set = res_player_idle_handle;
            P->animation->end = res_player_idle->count;
            P->animation->sprite = 0;
            break;
        case ID_Walk:
            P->animation->set = res_player_walk_handle;
            P->animation->end = res_player_walk->count;
            P->animation->sprite = 0;
            break;
        }
    }

}

#if 0
internal void
player_step(ProgramInput *input, Entity *P)
{
    player_vertical(input, P);

    if (button_ended_down(input, Button_Right)) {
        fsm_event(&input_fsm, ID_InputRightPressed, 0);
    } else if (button_ended_down(input, Button_Left)) {
        fsm_event(&input_fsm, ID_InputLeftPressed, 0);
    } else if (button_ended_up(input, Button_Right)) {
        fsm_event(&input_fsm, ID_InputRightReleased, 0);
    } else if (button_ended_up(input, Button_Left)) {
        fsm_event(&input_fsm, ID_InputLeftReleased, 0);
    }

    if (input_fsm.state == ID_KeyNone) {
        if (button_down(input, Button_Right)) {
            fsm_event(&input_fsm, ID_InputRightPressed, 0);
        }
        if (button_down(input, Button_Left)) {
            fsm_event(&input_fsm, ID_InputLeftPressed, 0);
        }
    }

    u32 state_previous = player_fsm.state;

    if (player_fsm.timeout) {
        player_fsm.timeout--;
        if (player_fsm.timeout == 0) {
            fsm_event(&player_fsm, ID_TimeOut, 1);
        }
    }

    fsm_event(&player_fsm, input_fsm.state, 1);

    if (player_fsm.state != state_previous)
    {
        player_fsm.timeout = 0;
        switch (player_fsm.state)
        {
        case ID_IdleLeft:
            P->vx = 0;
            entity_flip(P, 1);
            break;
        case ID_IdleRight:
            P->vx = 0;
            entity_flip(P, 0);
            break;

        case ID_IdleTurnLeft:
            player_fsm.timeout = 2;
            break;
        case ID_IdleTurnRight:
            player_fsm.timeout = 2;
            break;

        case ID_BeginWalkLeft:
            player_fsm.timeout = 2;
            P->vx = -1;
            break;
        case ID_BeginWalkRight:
            player_fsm.timeout = 2;
            P->vx = 1;
            break;

        case ID_EndWalkLeft:
            player_fsm.timeout = 2;
            P->vx = -1;
            break;
        case ID_EndWalkRight:
            player_fsm.timeout = 2;
            P->vx = 1;
            break;

        case ID_WalkLeft:
            P->vx = -2;
            entity_flip(P, 1);
            break;
        case ID_WalkRight:
            P->vx = 2;
            entity_flip(P, 0);
            break;

        case ID_WalkTurnLeft:
            player_fsm.timeout = 2;
            P->vx = 1;
            break;
        case ID_WalkTurnRight:
            player_fsm.timeout = 2;
            P->vx = -1;
            break;

        }
    }
}
#endif

#if 0
internal void
player_step(ProgramInput *input, Entity *P)
{
    static Collision C;
    // P->flags = C.B ? (P->flags | EntityFlag_Grounded) : (P->flags & ~EntityFlag_Grounded)

    u32 flags = P->flags;
    if (P->vy < 0) {
        flag_toggle_if(P->flags,
                       world_test_move(&GAME->world, P->collider, 0, -0.01f, CollisionTestMask_TileMap, &C, collision_stop)->B,
                       EntityFlag_Ceil);
        if (P->flags & EntityFlag_Ceil) {
            // TODO: Head hit animation + particles.
        }
    } else {
        // TODO: This might also need a forward/backward shift of the collider, so character
        //       cannot stand one px on the cliff, and also to be able to provide animation for
        //       standing on the ledge.
        flag_toggle_if(P->flags,
                       world_test_move(&GAME->world, P->collider, 0, 0.01f, CollisionTestMask_TileMap, &C, collision_stop)->B,
                       EntityFlag_Floor);
        if (P->flags & EntityFlag_Floor) {
            // TODO: Floor hit animation + particles.
        }
    }

    if (ceil) {
        P->vy = 0;
    } else if (floor) {
        P->vy = 0;
        if (button_ended_down(input, Button_X)) {
            P->vy = -8;
        }
    } else {
        // Fall
        P->vy = MIN(5, P->vy + 1);
    }

    i32 dx = 0;
    if (button_down(input, Button_Right)) dx = 1;
    if (button_down(input, Button_Left))  dx = -1;

    if (sign_of(P->vx) != dx)
    {
        if (dx == 0) {
            player_state_set(P, EntityState_Brake);
        } else {
            entity_flip(P, (dx < 0));
            P->dx = dx;
            player_state_set(P, EntityState_Walk);
        }
    }

    // vx and smoothing.
    i32 fx = dx * 2;
    if (fx > P->vx) {
        P->vx++;
    } else if (fx < P->vx) {
        P->vx--;
    }

    // Transition from Brake to Idle
    if (P->state == EntityState_Brake && dx == 0 && P->vx == 0) {
        player_state_set(P, EntityState_Idle);
    }

    // printf("vx %d vy %d x %.3f y %.3f\n", P->vx, P->vy, P->collider->x, P->collider->y);
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
            camera->vx = sign_of(P->vx) * (abs(P->vx)) + (dx / 7);
        } else {
            camera->vx = sign_of(P->vx);
        }
    } else {
        if (P->vx) {
            camera->vx = P->vx + (dx / 5);
        } else {
            camera->vx = dx / 5;
        }
    }

    camera->x += camera->vx;
    camera->y = P->collider->y - 12;
}
