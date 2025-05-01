#include "rigel.h"
#include "game.h"
#include "resource.h"
#include "json.h"
#include "render.h"

namespace rigel {

EntityPrototype entity_prototypes[EntityType_NumberOfTypes];
ZoneTriggerAction trigger_action_table[ZoneTriggerActions_NumActions];

void load_entity_prototypes(mem::GameMem& memory, const char* filepath)
{
    TextResource entity_protos = load_text_resource(filepath);
    auto root_v = parse_json_string(&memory.scratch_arena, entity_protos.text);
    assert(root_v->type == JSON_OBJECT && "couldn't parse entity protos");
    auto root = root_v->object;
    char path_buf[256];

    // load entity prototypes
    for (usize type = 0;
         type < EntityType_NumberOfTypes;
         type++)
    {
        const char* key = get_str_for_entity_type((EntityType)type);

        auto jsonv = jsonobj_get(root, key, strlen(key));
        auto obj = jsonv->object;

        auto entity_proto = entity_prototypes + type;
        auto entity_sprite = jsonobj_get(obj, "spritesheet", 11);
        json_str_copy(path_buf, entity_sprite->string);

        auto collider_width = jsonobj_get(obj, "collider_width", 14)->number->value;
        auto collider_height = jsonobj_get(obj, "collider_height", 15)->number->value;
        Rectangle entity_collider { 0, 0, collider_width, collider_height };

        // TODO: should load animations from json exported by aseprite
        auto frames = jsonobj_get(obj, "frames", 6)->number->value;
        auto resource = get_image_resource(path_buf);
        if (resource.resource_id != RESOURCE_ID_NONE) {
            entity_proto->spritesheet = resource;
        } else {
            entity_proto->spritesheet = load_image_resource(path_buf, frames);
        }
        entity_proto->collider_dims = entity_collider;
    }
}

WorldChunk* load_all_world_chunks(mem::GameMem& memory)
{
    // TODO: we should be getting chunk indexes from a resource
    // archive of some sort. We're nearing the point where we're
    // ready to start crafting a format for that.
    WorldChunk* starting_chunk = load_world_chunk(memory, "resource/map/layered2.tmj");
    starting_chunk->level_index = 0;

    memory.scratch_arena.reinit_zeroed();
    WorldChunk* other_chunk = load_world_chunk(memory, "resource/map/next.tmj");
    other_chunk->level_index = 1;

    return starting_chunk;
}

void switch_world_chunk(mem::GameMem& mem, GameState* state, i32 index)
{
    auto next_chunk = state->first_world_chunk + index;
    auto active_chunk = state->active_world_chunk;
    auto player = active_chunk->entities + active_chunk->player_id;

    if (next_chunk->player_id == ENTITY_ID_NONE) {
        next_chunk->player_id = next_chunk->add_entity(mem, player->type, player->position);
    } else {
        auto other_player = next_chunk->entities + next_chunk->player_id;
        other_player->state_flags = player->state_flags;
        other_player->position = player->position;
        other_player->velocity = player->velocity;
        other_player->acceleration = player->acceleration;
        other_player->facing_dir = player->facing_dir;
    }

    state->active_world_chunk = next_chunk;
    render::make_world_chunk_renderable(&mem.scratch_arena, next_chunk);
}

GameState*
load_game(mem::GameMem& memory)
{
    GameState* result = initialize_game_state(memory);

    load_entity_prototypes(memory, "resource/entity/entities.json");

    result->first_world_chunk = load_all_world_chunks(memory);
    result->active_world_chunk = result->first_world_chunk;

    render::make_world_chunk_renderable(&memory.scratch_arena, result->active_world_chunk);

    return result;
}

} // namespace rigel
