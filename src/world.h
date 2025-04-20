#ifndef RIGEL_WORLD_H_
#define RIGEL_WORLD_H_

#include "mem.h"
#include "rigel.h"
#include "entity.h"
#include "tilemap.h"
#include "resource.h"
#include "rigelmath.h"

#include <iostream>

namespace rigel {


///////////////////////////////////////////////////
// TODO: We need to re-write basically all of this.
// We wanna keep some of the level loading stuff tho.
///////////////////////////////////////////////////
#define MAX_ENTITIES 64

struct EntityHash
{
    EntityId id;
    usize index;
};

struct WorldChunk
{
    TileMap* active_map;

    // TODO: we will have _so few_ entities in one chunk.
    // Just do the list? Or have a free list?
    usize next_free_entity_idx;
    EntityHash entity_hash[MAX_ENTITIES];
    Entity entities[MAX_ENTITIES];


    // REVIEW
    EntityId add_entity(mem::GameMem& mem,
                        EntityType type,
                        SpriteResourceId sprite_id,
                        m::Vec3 initial_position,
                        Rectangle collider);
    // REVIEW
    Entity* add_player(mem::GameMem& mem,
                               SpriteResourceId sprite_id,
                               m::Vec3 initial_position,
                               Rectangle collider);
};

WorldChunk*
load_world_chunk(mem::GameMem& mem);

} // namespace rigel
#endif // RIGEL_WORLD_H_
