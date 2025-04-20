#include "world.h"
#include "tilemap.h"
#include "rigelmath.h"
#include <tinyxml2.h>

#include <iostream>


namespace rigel {

WorldChunk*
load_world_chunk(mem::GameMem& mem)
{
    WorldChunk* result = mem.game_state_arena.alloc_simple<WorldChunk>();
    // TODO: this should be something that can hold lots of them I think
    mem::Arena tilemap_arena = mem.stage_arena.alloc_sub_arena(18*ONE_KB);

    TextResource tilemap_data = load_text_resource("resource/map/layered2.tmx");

    result->active_map = load_tile_map_from_xml(tilemap_arena, tilemap_data);

    for (int i = 0; i < MAX_ENTITIES; i++) {
        result->entity_hash[i].id = ENTITY_ID_NONE;
        result->entities[i].id = ENTITY_ID_NONE;
        result->entities[i].state_flags = STATE_DELETED;
    }
    return result;
}

EntityId
WorldChunk::add_entity(mem::GameMem& mem,
                       EntityType type,
                       SpriteResourceId sprite_id,
                       m::Vec3 initial_position,
                       Rectangle collider)
{
    // TODO: allocating these is likely more tricky
    EntityId entity_id = next_free_entity_idx;

    usize hash = entity_id & (MAX_ENTITIES - 1);
    entity_hash[hash].id = entity_id;
    entity_hash[hash].index = next_free_entity_idx;
    next_free_entity_idx++;

    Entity* new_entity = &entities[entity_id];
    new_entity->id = entity_id;
    new_entity->type = type;
    new_entity->sprite_id = sprite_id;
    new_entity->position = initial_position;

    auto colliders = mem.colliders_arena.alloc_simple<ColliderSet>();
    colliders->n_aabbs = 1;
    colliders->aabbs = mem.colliders_arena.alloc_array<AABB>(1);
    colliders->aabbs[0] = aabb_from_rect(collider);
    new_entity->colliders = colliders;

    return entity_id;
}

Entity*
WorldChunk::add_player(mem::GameMem& mem,
                       SpriteResourceId sprite_id,
                       m::Vec3 initial_position,
                       Rectangle collider)
{
    entity_hash[0].id = PLAYER_ENTITY_ID;
    entity_hash[0].index = next_free_entity_idx;

    Entity* new_entity = &entities[next_free_entity_idx];
    next_free_entity_idx++;
    new_entity->id = PLAYER_ENTITY_ID;
    new_entity->type = ENTITY_TYPE_PLAYER;
    new_entity->state_flags &= ~(STATE_DELETED);
    new_entity->sprite_id = sprite_id;
    new_entity->position = initial_position;

    // TODO: I need an easier storage scheme here. We should have collider
    // ID's I think? I need a way to identify when two entities use the
    // same collider set.
    auto colliders = mem.colliders_arena.alloc_simple<ColliderSet>();
    colliders->n_aabbs = 1;
    colliders->aabbs = mem.colliders_arena.alloc_array<AABB>(1);
    colliders->aabbs[0] = aabb_from_rect(collider);
    new_entity->colliders = colliders;

    return new_entity;
}

}
