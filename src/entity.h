#ifndef ENTITY_H_
#define ENTITY_H_

#include <iostream>

#include "rigel.h"
#include "rigelmath.h"
#include "collider.h"
#include "resource.h"
#include "tilemap.h"
#include "timer.h"

namespace rigel {

#define ANIMATION_IDLE "idle"
#define ANIMATION_WALK "walk"

enum EntityState
{
    // movement
    STATE_ON_LAND = 0x1,
    STATE_JUMPING = 0x2,
    STATE_FALLING = 0x4,

    // damage
    STATE_IFRAMES = 0x10,

    STATE_DELETED = 0x80000000
};

// TODO: there's gotta be a more flex way to do this, ya?
// could I specify entity prototypes in an external text file,
// perhaps? Do I need to?
enum EntityType
{
    EntityType_Player = 0,
    EntityType_Bumpngo,
    EntityType_NumberOfTypes
};

inline const char*
get_str_for_entity_type(EntityType type)
{
    switch (type) {
        case EntityType_Player:
        {
            return "EntityType_Player";
        } break;
        case EntityType_Bumpngo:
        {
            return "EntityType_Bumpngo";
        } break;
        default:
        {
            return nullptr;
        }
    }
}

inline EntityType
get_entity_type_for_str(const char* type)
{
    if (strncmp(type, "EntityType_Player", 17) == 0) {
        return EntityType_Player;
    } else if (strncmp(type, "EntityType_Bumpngo", 18) == 0) {
        return EntityType_Bumpngo;
    } else {
        return EntityType_NumberOfTypes;
    }
}

struct PlayingAnimation
{
    usize start_frame;
    usize end_frame;
    usize current_frame;
    // TODO: proper timers!
    f32 timer;
    f32 threshold;
};

struct Entity
{
    EntityId id;
    EntityType type;
    ResourceId sprite_id;
    i32 new_sprite_id;

    // sooo we have these state flags which means that an
    // entity can be in more than one state at a time. I think
    // this was initialially done because jumping and falling
    // are technically co-incident. Buuuuut I am now thinking
    // that this is silly.
    EntityState state;

    m::Vec3 position;
    m::Vec3 velocity;
    m::Vec3 acceleration;

    ZeroCrossTrigger facing_dir;
    ResourceId animations_id;
    const char* current_animation;
    PlayingAnimation animation;

    // TODO: this should just be a rectangle. No multiple colliders.
    ColliderSet* colliders;
};

struct EntityMoveResult
{
    bool collision_happened;
    bool collided[9];
};

EntityMoveResult
move_entity(Entity* entity, TileMap* tile_map, f32 dt, f32 top_speed = 120);
bool
collides_with_level(AABB aabb, TileMap* tile_map);
void
entity_set_animation(Entity* entity, const char* anim_name);
void
entity_update_animation(Entity* entity, const char* anim_name);

inline bool state_transition_air_to_land(Entity* e)
{
    if (e->state == STATE_JUMPING || e->state == STATE_FALLING)
    {
        e->state = STATE_ON_LAND;
        return true;
    }
    return false;
}

inline bool state_transition_land_to_jump(Entity* e)
{
    if (e->state == STATE_ON_LAND)
    {
        e->state = STATE_JUMPING;
        return true;
    }
    return false;
}

inline bool state_transition_land_to_fall(Entity* e)
{
    if (e->state == STATE_ON_LAND)
    {
        e->state = STATE_FALLING;
        return true;
    }
    return false;
}

inline bool state_transition_fall_exclusive(Entity* e)
{
    e->state = STATE_FALLING;
    return true;
}



struct EntityPrototype
{
    ImageResource spritesheet;
    i32 new_sprite_id;
    ResourceId animation_id;
    Rectangle collider_dims;
};


} // namespace rigel

#endif // ENTITY_H_
