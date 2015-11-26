#ifndef MAIN_H
#define MAIN_H

#include "world.h"

typedef struct
{
    ImageSet *set;
    u16 begin;
    u16 end;

    u16 sprite;
    u8 sprite_mode;
}
Animation;

typedef enum
{
    EntityState_Idle = 0,
    EntityState_Brake,
    EntityState_Walk
}
EntityState;

typedef struct
{
    u8 state;
    i32 dx;
    i32 vx;
    i32 vy;
    i32 sprite_ox;
    i32 sprite_oy;
    Collider *collider;
    Animation *animation;
}
Entity;

#define entity_center_x(E) \
    ((E)->collider->x)

#define entity_center_y(E) \
    ((E)->collider->y)

#define entity_left(E) \
    ((E)->collider->x - (E)->collider->w)

#define entity_top(E) \
    ((E)->collider->y - (E)->collider->h)

#define entity_right(E) \
    ((E)->collider->x + (E)->collider->w)

#define entity_bottom(E) \
    ((E)->collider->y + (E)->collider->h)

#define entity_flip(E, On) E->animation->sprite_mode = On \
    ? (E->animation->sprite_mode |  DrawSpriteMode_FlipH) \
    : (E->animation->sprite_mode & ~DrawSpriteMode_FlipH)

typedef struct
{
    i32 x;
    i32 y;
    i32 vx;
}
Camera;

#define static_array(Type, Name, Count) \
    Type Name[Count]; \
    u32 Name##_count

#define static_array_push(Array, Count) \
    ( Array + (Array##_count++) )

typedef struct
{
    Camera camera;

    static_array(Animation, animations, WORLD_COLLIDERS_MAX);
    static_array(Entity, entities, WORLD_COLLIDERS_MAX);

    World world;

    Entity *player;
}
ProgramState;

#endif // MAIN_H

