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

enum EntityStateFlag
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


struct Entity
{
    EntityId id;
    EntityType type;
    SpriteResourceId sprite_id;

    u32 state_flags;

    m::Vec3 position;
    m::Vec3 velocity;
    m::Vec3 acceleration;

    ZeroCrossTrigger facing_dir;

    // TODO: this should just be a rectangle. No multiple colliders.
    ColliderSet* colliders;
};

void
move_entity(Entity* entity, TileMap* tile_map, f32 dt);
bool
collides_with_level(AABB aabb, TileMap* tile_map);

inline void state_transition_air_to_land(Entity* e)
{
    u32 flags = e->state_flags;
    flags &= ~(STATE_JUMPING | STATE_FALLING);
    flags |= STATE_ON_LAND;
    e->state_flags = flags;
}

inline void state_transition_land_to_jump(Entity* e)
{
    u32 flags = e->state_flags;
    flags &= ~(STATE_ON_LAND);
    flags |= STATE_JUMPING;
    e->state_flags = flags;
}

inline void state_transition_land_to_fall(Entity* e)
{
    u32 flags = e->state_flags;
    flags &= ~(STATE_ON_LAND);
    flags |= STATE_FALLING;
    e->state_flags = flags;
}

inline void state_transition_fall_exclusive(Entity* e)
{
    u32 flags = e->state_flags;
    flags &= ~(STATE_ON_LAND | STATE_JUMPING);
    flags |= STATE_FALLING;
    e->state_flags = flags;
}

struct EntityPrototype
{
    ImageResource spritesheet;
    Rectangle collider_dims;
};


} // namespace rigel

#endif // ENTITY_H_
