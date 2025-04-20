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
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_BUMPNGO
};

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


} // namespace rigel

#endif // ENTITY_H_
