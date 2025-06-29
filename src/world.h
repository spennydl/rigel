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
    LightType_Circle,
    LightType_NLightTypes
};

inline LightType
get_light_type_for_str(const char* str)
{
    if (strcmp(str, "LightType_Point") == 0)
    {
        return LightType_Point;
    }
    if (strcmp(str, "LightType_Circle") == 0)
    {
        return LightType_Circle;
    }
    return LightType_NLightTypes;
}


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

    i32 next_free_entity_idx;
    EntityHash entity_hash[MAX_ENTITIES];
    Entity entities[MAX_ENTITIES];
    EntityId player_id;

    i32 next_free_light_idx;
    Light lights[24];

    ZoneTriggerData zone_triggers[16];

    // REVIEW
    EntityId add_entity(mem::GameMem& mem,
                        EntityType type,
                        m::Vec3 initial_position);

    u32 add_light(LightType type, m::Vec3 position, m::Vec3 color);

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
