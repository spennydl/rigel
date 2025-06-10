#ifndef RIGEL_WORLD_H_
#define RIGEL_WORLD_H_

#include "mem.h"
#include "rigel.h"
#include "entity.h"
#include "collider.h"
#include "tilemap.h"
#include "resource.h"
#include "rigelmath.h"
#include "trigger.h"

#include <iostream>

namespace rigel {


///////////////////////////////////////////////////
// TODO: We need to re-write basically all of this.
// We wanna keep some of the level loading stuff tho.
///////////////////////////////////////////////////
#define MAX_ENTITIES 64

extern EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

struct EntityHash
{
    EntityId id;
    usize index;
};

enum LightType
{
    LightType_Point,
    LightType_Circle
};

struct Light
{
    LightType type;
    m::Vec3 position;
    m::Vec3 color;
};

struct WorldChunk
{
    i32 level_index;
    TileMap* active_map;

    usize next_free_entity_idx;
    EntityHash entity_hash[MAX_ENTITIES];
    Entity entities[MAX_ENTITIES];
    EntityId player_id;

    Light lights[24];

    ZoneTriggerData zone_triggers[16];

    // REVIEW
    EntityId add_entity(mem::GameMem& mem,
                        EntityType type,
                        m::Vec3 initial_position);

};

struct EntityIterator
{
    EntityIterator(WorldChunk* wc)
    {
        first = wc->entities;
        current = first;
        last = wc->entities + wc->next_free_entity_idx;
    }

    Entity* begin()
    {
        return current;
    }

    Entity* end()
    {
        return last;
    }

    Entity* next()
    {
        current++;
        while (current < last && first->state == STATE_DELETED)
        {
            current++;
        }
        return current;
    }

    Entity* first;
    Entity* current;
    Entity* last;
};

WorldChunk*
load_world_chunk(mem::GameMem& mem, const char* file_path);

inline Entity*
get_player(WorldChunk* wc)
{
    if (wc->player_id != ENTITY_ID_NONE)
    {
        return wc->entities + wc->player_id;
    }
    return nullptr;
}


} // namespace rigel
#endif // RIGEL_WORLD_H_
