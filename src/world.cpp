#include "world.h"
#include <tinyxml2.h>

#include <iostream>
#include <sstream>


namespace rigel {

WorldChunk*
load_world_chunk(mem::GameMem& mem)
{
    WorldChunk* result = mem.game_state_arena.alloc_simple<WorldChunk>();
    // TODO: this should be something that can hold lots of them I think
    mem::Arena tilemap_arena = mem.stage_arena.alloc_sub_arena(6*ONE_KB);

    TextResource tilemap_data = load_text_resource(mem.resource_arena, "resource/map/layeredlevel.tmx");

    result->active_map = load_tile_map_from_xml(tilemap_arena, tilemap_data);

    for (int i = 0; i < MAX_ENTITIES; i++) {
        result->entity_hash[i].id = ENTITY_ID_NONE;
        result->entities[i].id = ENTITY_ID_NONE;
        result->entities[i].state_flags = entity::STATE_DELETED;
    }
    return result;
}

EntityId
WorldChunk::add_entity(mem::GameMem& mem,
                       entity::EntityType type,
                       SpriteResourceId sprite_id,
                       glm::vec3 initial_position,
                       Rectangle collider)
{
    // TODO: allocating these is likely more tricky
    EntityId entity_id = next_free_entity_idx;

    usize hash = entity_id & (MAX_ENTITIES - 1);
    entity_hash[hash].id = entity_id;
    entity_hash[hash].index = next_free_entity_idx;
    next_free_entity_idx++;

    entity::Entity* new_entity = &entities[entity_id];
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

entity::Entity*
WorldChunk::add_player(mem::GameMem& mem,
                       SpriteResourceId sprite_id,
                       glm::vec3 initial_position,
                       Rectangle collider)
{
    entity_hash[0].id = PLAYER_ENTITY_ID;
    entity_hash[0].index = next_free_entity_idx;

    entity::Entity* new_entity = &entities[next_free_entity_idx];
    next_free_entity_idx++;
    new_entity->id = PLAYER_ENTITY_ID;
    new_entity->type = entity::ENTITY_TYPE_PLAYER;
    new_entity->state_flags &= ~(entity::STATE_DELETED);
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

auto
operator<<(std::ostream& os, const TileType& tt) -> std::ostream&
{
    switch (tt) {
        case TileType::EMPTY:
            os << "EMPTY";
            break;
        case TileType::WALL:
            os << "WALL";
            break;
        default:
            os << "ERRRRRRRR";
            break;
    }
    return os;
}

void
parse_tile_csv(const char* csv, TileMap* map)
{
    std::string line;
    std::size_t rows = 0;
    std::size_t cols = 0;

    int map_size = WORLD_SIZE_TILES;

    // glm::mat4* transforms = new glm::mat4[MAP_SIZE];

    std::istringstream csvin(csv);
    while (std::getline(csvin, line)) {
        if ((rows * WORLD_WIDTH_TILES) + cols >= (WORLD_WIDTH_TILES * WORLD_HEIGHT_TILES)) {
            std::cerr << "ERR: Overflowed tile map!" << std::endl;
            std::cerr << rows << " x " << cols << std::endl;
            return;
        }

        if (line.size() == 0) {
            continue;
        }

        std::istringstream line_ss(line);
        std::string tile_i;
        while (std::getline(line_ss, tile_i, ',')) {
            int tileno;
            if (std::istringstream(tile_i) >> tileno) {
                i32 idx = (rows * WORLD_WIDTH_TILES) + cols;
                map->tile_sprites[idx] = tileno;
                map->tiles[idx] = (tileno == 0) ? TileType::EMPTY : TileType::WALL;
            } else {
                std::cerr << "WARN: unexpected value in tile map csv"
                          << std::endl;
            }
            cols++;
        }

        cols = 0;
        rows++;
    }
}

void
dump_tile_map(const TileMap* tilemap)
{
    for (std::size_t row = 0; row < WORLD_HEIGHT_TILES; row++) {
        for (std::size_t col = 0; col < WORLD_WIDTH_TILES; col++) {
            auto idx = (row * WORLD_WIDTH_TILES) + col;
            std::cout << tilemap->tiles[idx] << ":" << tilemap->tile_sprites[idx]
                      << " ";
        }
        std::cout << std::endl;
    }
}

TileMap*
load_tile_map_from_xml(mem::Arena& arena, TextResource xml_data)
{
    tinyxml2::XMLDocument doc;
    doc.Parse(xml_data.text, xml_data.length);

    auto map = doc.FirstChildElement("map");
    assert(map && "No map!");

    auto width = map->IntAttribute("width");
    auto height = map->IntAttribute("height");

    assert(width == WORLD_WIDTH_TILES && "ERR: width is wrong");
    assert(height == WORLD_HEIGHT_TILES && "ERR: height is wrong");

    TileMap* tile_map = arena.alloc_simple<TileMap>();

    auto map_el = map->FirstChildElement("layer");
    while (true) {
        if (strncmp(map_el->Attribute("name"), "fg", 2) == 0) {
            auto data = map_el->FirstChildElement("data")->GetText();
            parse_tile_csv(data, tile_map);
            break;
        }
        map_el = map_el->NextSiblingElement("layer");
    }
    assert(map_el && "Couldn't find FG layer in map");

    return tile_map;
}

}
