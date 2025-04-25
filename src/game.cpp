#include "rigel.h"
#include "game.h"
#include "resource.h"
#include "json.h"

namespace rigel {

EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

GameState*
load_game(mem::GameMem& memory)
{
    TextResource entity_protos = load_text_resource("resource/entity/entities.json");
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

    GameState* result = initialize_game_state(memory);
    return result;
}

} // namespace rigel
