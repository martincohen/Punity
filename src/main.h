#ifndef MAIN_H
#define MAIN_H

#include "punity_world.h"

enum CustomAssetType
{
    AssetType_Level = AssetType_Custom + 1
};

typedef struct Spawn
{
    u32 type;
    V2i position;
}
Spawn;

typedef struct Level
{
    TileMap *layers[8];
    u32 layers_count;

    TileMap *map_collision;
    TileMap *map_meta;

    ImageSet *set_main;
    ImageSet *set_meta;

    V2i player_start;
    Spawn spawns[256];
    u32 spawn_count;
}
Level;

typedef struct Animation
{
    ImageSet *set;
    u16 begin;
    u16 end;

    u16 sprite;
    u8 sprite_mode;
}
Animation;

typedef enum EntityState
{
    EntityState_Idle = 0,
    EntityState_Brake,
    EntityState_Walk
}
EntityState;

typedef enum EntityFlags
{
    EntityFlag_Grounded = 0x01
}
EntityFlags;

typedef struct Entity
{
    u8 state;
    u8 flags;
    i32 dx;
    i32 vx;
    f32 vy;
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

typedef struct Camera
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

typedef struct ProgramState
{
    Camera camera;

    static_array(Animation, animations, WORLD_COLLIDERS_MAX);
    static_array(Entity, entities, WORLD_COLLIDERS_MAX);

    World world;

    Entity *player;
}
ProgramState;

extern ProgramState *GAME;

#endif // MAIN_H

