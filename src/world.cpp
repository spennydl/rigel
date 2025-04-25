#include "world.h"
#include "tilemap.h"
#include "rigelmath.h"
#include "json.h"

#include <iostream>


namespace rigel {

WorldChunk*
load_world_chunk(mem::GameMem& mem)
{
    WorldChunk* result = mem.game_state_arena.alloc_simple<WorldChunk>();

    for (int i = 0; i < MAX_ENTITIES; i++) {
        result->entity_hash[i].id = ENTITY_ID_NONE;
        result->entities[i].id = ENTITY_ID_NONE;
        result->entities[i].state_flags = STATE_DELETED;
    }

    // TODO: this should be something that can hold lots of them I think
    mem::Arena tilemap_arena = mem.stage_arena.alloc_sub_arena(18*ONE_KB);

    TextResource world_data = load_text_resource("resource/map/layered2.tmj");
    auto root_obj_v = parse_json_string(&mem.scratch_arena, world_data.text);
    assert(root_obj_v->type == JSON_OBJECT && "expect an object at root");
    auto root = root_obj_v->object;

    TileMap* tile_map = tilemap_arena.alloc_simple<TileMap>();
    TileMap* decoration = tilemap_arena.alloc_simple<TileMap>();
    TileMap* background = tilemap_arena.alloc_simple<TileMap>();
    tile_map->background = background;
    tile_map->decoration = decoration;

    result->active_map = tile_map;

    auto width = jsonobj_get(root, "width", 5);
    assert(width->type == JSON_NUMBER && "width is not a number");
    auto height = jsonobj_get(root, "height", 6);
    assert(height->type == JSON_NUMBER && "height is not a number");

    assert(width->number->value == WORLD_WIDTH_TILES && "ERR: width is wrong");
    assert(height->number->value == WORLD_HEIGHT_TILES && "ERR: height is wrong");

    auto layers_v = jsonobj_get(root, "layers", 6);
    assert(layers_v->type == JSON_OBJECT_ARRAY && "expect object array");
    auto head = layers_v->obj_array;

    bool fg = false;
    bool bg = false;
    bool dec = false;
    bool entities = true;
    bool lights = true;

    while (head)
    {
        auto obj = head->obj;
        auto name_v = jsonobj_get(obj, "name", 4);
        assert(name_v->type == JSON_STRING && "name is not a string");
        auto name = name_v->string;

        if (json_str_equals(*name, "fg", 2))
        {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY && "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray works..
            fill_tilemap_from_array(tile_map, reinterpret_cast<f32*>(data->arr), data->n);
            fg = true;
        }
        else if (json_str_equals(*name, "bg", 2))
        {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY && "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray works..
            fill_tilemap_from_array(background, reinterpret_cast<f32*>(data->arr), data->n);
            bg = true;
        }
        else if (json_str_equals(*name, "decoration", 10))
        {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY && "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray works..
            fill_tilemap_from_array(decoration, reinterpret_cast<f32*>(data->arr), data->n);
            dec = true;
        }
        else if (json_str_equals(*name, "entities", 8))
        {
            entities = true;
        }
        else if (json_str_equals(*name, "lights", 6))
        {
            lights = true;
        }
        else
        {
            assert(false && "Found an unexpected layer");
        }
        head = head->next;
    }

    assert((fg && bg && dec && entities && lights) && "missing a layer");

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
