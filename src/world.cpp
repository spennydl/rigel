#include "world.h"
#include "json.h"
#include "rigelmath.h"
#include "tilemap.h"

#include <iostream>

namespace rigel {

m::Vec3
parse_color_str(const char* str)
{
    assert(str[0] == '#' && "malformed color");
    str = str + 1;
    u32 mask = 0x000000FF;

    u32 val = strtol(str, nullptr, 16);

    m::Vec3 result;
    for (i32 i = 2; i >= 0; i--)
    {
        result.xyz[i] = (float)(val & mask) / 255.0f;
        val >>= 8;
    }

    return result;
}

WorldChunk*
load_world_chunk(mem::GameMem& mem, const char* file_path)
{
    WorldChunk* result = mem.game_state_arena.alloc_simple<WorldChunk>();

    for (int i = 0; i < MAX_ENTITIES; i++) {
        result->entity_hash[i].id = ENTITY_ID_NONE;
        result->entities[i].id = ENTITY_ID_NONE;
        result->entities[i].state = STATE_DELETED;
    }
    result->player_id = ENTITY_ID_NONE;

    // TODO: this should be something that can hold lots of them I think
    mem::Arena tilemap_arena = mem.stage_arena.alloc_sub_arena(18 * ONE_KB);

    TextResource world_data = load_text_resource(file_path);
    auto root_obj_v = parse_json_string(&mem.scratch_arena, world_data.text);
    assert(root_obj_v->type == JSON_OBJECT && "expect an object at root");
    auto root = root_obj_v->object;

    TileMap* tile_map = tilemap_arena.alloc_simple<TileMap>();
    TileMap* decoration = tilemap_arena.alloc_simple<TileMap>();
    TileMap* background = tilemap_arena.alloc_simple<TileMap>();

    ImageResource tilesheet = get_or_load_image_resource("resource/image/tiles_merged.png");
    tile_map->tile_sheet = tilesheet.resource_id;
    decoration->tile_sheet = tilesheet.resource_id;
    background->tile_sheet = tilesheet.resource_id;

    tile_map->background = background;
    tile_map->decoration = decoration;
    result->active_map = tile_map;

    auto width = jsonobj_get(root, "width", 5);
    assert(width->type == JSON_NUMBER && "width is not a number");
    auto height = jsonobj_get(root, "height", 6);
    assert(height->type == JSON_NUMBER && "height is not a number");

    assert(width->number->value == WORLD_WIDTH_TILES && "ERR: width is wrong");
    assert(height->number->value == WORLD_HEIGHT_TILES &&
           "ERR: height is wrong");

    auto layers_v = jsonobj_get(root, "layers", 6);
    assert(layers_v->type == JSON_OBJECT_ARRAY && "expect object array");
    auto head = layers_v->obj_array;

    bool fg = false;
    bool bg = false;
    bool dec = false;
    bool entities = true;
    bool lights = true;

    while (head) {
        auto obj = head->obj;
        auto name_v = jsonobj_get(obj, "name", 4);
        assert(name_v->type == JSON_STRING && "name is not a string");
        auto name = name_v->string;

        if (json_str_equals(*name, "fg", 2)) {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY &&
                   "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray
            // works..
            fill_tilemap_from_array(
              tile_map, reinterpret_cast<f32*>(data->arr), data->n);
            fg = true;
        } else if (json_str_equals(*name, "bg", 2)) {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY &&
                   "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray
            // works..
            fill_tilemap_from_array(
              background, reinterpret_cast<f32*>(data->arr), data->n);
            bg = true;
        } else if (json_str_equals(*name, "decoration", 10)) {
            auto data_v = jsonobj_get(obj, "data", 4);
            assert(data_v->type == JSON_NUMBER_ARRAY &&
                   "data is not a numeric array");
            auto data = data_v->number_array;

            // TODO: the cast kind of sucks. Should re-think how jsonarray
            // works..
            fill_tilemap_from_array(
              decoration, reinterpret_cast<f32*>(data->arr), data->n);
            dec = true;
        } else if (json_str_equals(*name, "entities", 8)) {
            auto objects_v = jsonobj_get(obj, "objects", 7);
            // TODO: I need a better way to detect empty arrays
            if (objects_v->type != JSON_OBJECT_ARRAY) {
                head = head->next;
                continue;
            }
            auto objects = objects_v->obj_array;

            while (objects) {
                auto obj = objects->obj;
                auto props = jsonobj_get(obj, "properties", 10)->obj_array;

                EntityType type = EntityType_NumberOfTypes;

                while (props) {
                    auto prop = props->obj;

                    auto name = jsonobj_get(prop, "name", 4)->string;
                    if (json_str_equals(*name, "Type", 4)) {
                        // TODO: jsonstrings reeeeeeally suck
                        char buf[64];
                        auto type_s = jsonobj_get(prop, "value", 5)->string;
                        json_str_copy(buf, type_s);
                        type = get_entity_type_for_str(buf);
                    }

                    props = props->next;
                }
                f32 x_pos = jsonobj_get(obj, "x", 1)->number->value;
                f32 y_pos = jsonobj_get(obj, "y", 1)->number->value;
                y_pos = (WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS) - y_pos;

                if (type != EntityType_NumberOfTypes) {
                    EntityId id =
                      result->add_entity(mem, type, m::Vec3{ x_pos, y_pos });

                    if (type == EntityType_Player) {
                        result->player_id = id;
                    }
                }
                objects = objects->next;
            }

            entities = true;
        } else if (json_str_equals(*name, "lights", 6)) {
            auto objects_v = jsonobj_get(obj, "objects", 7);
            if (objects_v->type != JSON_OBJECT_ARRAY) {
                head = head->next;
                continue;
            }
            auto objects = objects_v->obj_array;

            while(objects)
            {
                auto obj = objects->obj;
                auto props = jsonobj_get(obj, "properties", 10)->obj_array;

                LightType light_type = LightType_NLightTypes;
                m::Vec3 color{0, 0, 0};
                float intensity = -1;

                while (props)
                {
                    auto prop = props->obj;

                    auto name = jsonobj_get(prop, "name", 4)->string;
                    if (json_str_equals(*name, "LightType", 9))
                    {
                        char buf[64];
                        auto type_s = jsonobj_get(prop, "value", 5)->string;
                        json_str_copy(buf, type_s);
                        light_type = get_light_type_for_str(buf);
                    }
                    else if (json_str_equals(*name, "Color", 5))
                    {
                        char buf[32];
                        auto color_s = jsonobj_get(prop, "value", 5)->string;
                        json_str_copy(buf, color_s);
                        color = parse_color_str(buf);
                    }
                    else if (json_str_equals(*name, "Intensity", 9))
                    {
                        intensity = jsonobj_get(prop, "value", 5)->number->value;
                    }
                    props = props->next;
                }
                f32 x_pos = jsonobj_get(obj, "x", 1)->number->value;
                f32 y_pos = jsonobj_get(obj, "y", 1)->number->value;
                y_pos = (WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS) - y_pos;

                if(light_type != LightType_NLightTypes
                            && color != m::Vec3{0, 0, 0}
                            && intensity > 0)
                {
                    color = color * intensity;
                    result->add_light(light_type, m::Vec3{x_pos, y_pos, 0}, color);
                }
                else
                {
                    std::cout << "skipping light with not enough props..." << std::endl;
                }
                objects = objects->next;
            }
            lights = true;
        } else {
            assert(false && "Found an unexpected layer");
        }

        head = head->next;
    }

    assert((fg && bg && dec && entities && lights) && "missing a layer");

    return result;
}

u32 WorldChunk::add_light(LightType type, m::Vec3 position, m::Vec3 color)
{
    auto idx = next_free_light_idx;
    assert(idx < 24 && "too many lights!");
    next_free_light_idx++;

    auto light = lights + idx;
    light->type = type;
    light->position = position;
    light->color = color;

    return idx;
}

EntityId
WorldChunk::add_entity(mem::GameMem& mem,
                       EntityType type,
                       m::Vec3 initial_position)
{
    // TODO: allocating these is likely more tricky
    EntityId entity_id = next_free_entity_idx;

    usize hash = entity_id & (MAX_ENTITIES - 1);
    entity_hash[hash].id = entity_id;
    entity_hash[hash].index = next_free_entity_idx;
    next_free_entity_idx++;

    EntityPrototype proto = entity_prototypes[type];

    Entity* new_entity = &entities[entity_id];
    new_entity->id = entity_id;
    new_entity->type = type;
    new_entity->sprite_id = proto.spritesheet.resource_id;
    new_entity->position = initial_position;
    new_entity->animations_id = proto.animation_id;
    entity_set_animation(new_entity, "idle");

    // TODO: This is a thorn in my side
    auto colliders = mem.colliders_arena.alloc_simple<ColliderSet>();
    colliders->n_aabbs = 1;
    colliders->aabbs = mem.colliders_arena.alloc_array<AABB>(1);
    colliders->aabbs[0] = aabb_from_rect(proto.collider_dims);
    new_entity->colliders = colliders;

    return entity_id;
}

}
