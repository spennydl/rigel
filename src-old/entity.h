#ifndef ENTITY_H_
#define ENTITY_H_

#include <cmath>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

#include "rigel.h"
#include "collider.h"
#include "resource.h"

namespace rigel {
namespace entity {


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

    f32 jump_time;
    u32 state_flags;
    glm::vec3 position;
    // TODO: we should try to do without this one
    glm::vec3 position_err;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 forces;

    ColliderSet* colliders;
};

} // namespace entity
} // namespace rigel

#endif // ENTITY_H_
