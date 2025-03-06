#include "collider.h"
#include "world.h"
#include <iostream>

namespace rigel {

// I dislike Tiled's xml format. At least it's gonna go
// away when we have asset packing implemented.

void load_aabb_colliders(Stage* stage, tinyxml2::XMLElement* root, mem::GameMem& mem)
{
    auto obj = root->FirstChildElement("object");

    size_t n_objects = 0;
    while (obj) {
        n_objects++;
        obj = obj->NextSiblingElement("object");
    }
    obj = root->FirstChildElement("object");

    stage->colliders.n_aabbs = n_objects;
    stage->colliders.aabbs =
      mem.stage_arena.alloc_array<AABB>(n_objects);

    auto colliders = &stage->colliders;
    // load level colliders
    int i = 0;
    while (obj) {
        auto aabb = &colliders->aabbs[i];

        auto aabb_w_ex = obj->IntAttribute("width") / 2.0;
        auto aabb_h_ex = obj->IntAttribute("height") / 2.0;
        auto x = obj->IntAttribute("x");
        auto y = stage->dimensions.h - obj->IntAttribute("y") - (aabb_h_ex * 2.0);

        aabb->center = glm::vec3(x + aabb_w_ex, y + aabb_h_ex, 0);
        aabb->extents = glm::vec3(aabb_w_ex, aabb_h_ex, 0);

        obj = obj->NextSiblingElement("object");
        i++;
    }
    std::cout << "loaded " << i << " AABBs" << std::endl;
}

void load_tri_colliders(Stage* stage, tinyxml2::XMLElement* root, mem::GameMem& mem)
{
    auto obj = root->FirstChildElement("object");

    size_t n_objects = 0;
    while (obj) {
        n_objects++;
        obj = obj->NextSiblingElement("object");
    }
    obj = root->FirstChildElement("object");
    std::cout << "found " << n_objects << " tris";

    stage->colliders.n_tris = n_objects;
    stage->colliders.tris =
      mem.stage_arena.alloc_array<CollisionTri>(n_objects);

    auto colliders = &stage->colliders;
    // load level colliders
    int i = 0;
    while (obj) {
        auto tri = &colliders->tris[i];
        auto xml_tri = obj->FirstChildElement("polygon");
        f32 x = obj->FloatAttribute("x");
        f32 y = obj->FloatAttribute("y");

        char* offsets = (char*)xml_tri->Attribute("points");

        for (int j = 0; j < 3; j++) {
            char* end = offsets;
            while (*end && *end != ',')
                end++;

            f32 x_off = strtod(offsets, &end);
            offsets = end + 1;
            while (*end && *end != ' ')
                end++;

            f32 y_off = strtod(offsets, &end);
            offsets = end + 1;

            tri->vertices[j].x = x + x_off;
            tri->vertices[j].y = stage->dimensions.h - (y + y_off);
            //std::cout << tri->vertices[j].x << "," << tri->vertices[j].y << std::endl;
        }

        obj = obj->NextSiblingElement("object");
        i++;
    }
    std::cout << "loaded " << i << " tris" << std::endl;
}

auto
load_level_data(mem::GameMem& mem,
                WorldChunk* world_chunk,
                const char* chunk_name)
{
    mem.stage_arena.reinit();
    Stage* stage = mem.stage_arena.alloc_simple<Stage>();
    assert(stage && "Couldn't alloc a stage!");

    tinyxml2::XMLDocument doc;
    doc.LoadFile(chunk_name);

    auto map = doc.FirstChildElement("map");
    if (!map) {
        return;
    }
    //auto props = map->FirstChildElement("imagelayer")->FirstChildElement("image");
    auto width = map->IntAttribute("width") * map->IntAttribute("tilewidth");
    auto height = map->IntAttribute("height") * map->IntAttribute("tileheight");
    
    stage->dimensions.x = 0;
    stage->dimensions.y = 0;
    stage->dimensions.w = width;
    stage->dimensions.h = height;

    auto obj_group = map->FirstChildElement("objectgroup");
    auto group_name = obj_group->Attribute("name");
    std::cout << group_name << std::endl;

    if (strncmp(group_name, "AABB", 4) == 0) {
        load_aabb_colliders(stage, obj_group, mem);
    } else if (strncmp(group_name, "TRI", 3) == 0) {
        load_tri_colliders(stage, obj_group, mem);
    }

    obj_group = obj_group->NextSiblingElement("objectgroup");
    group_name = obj_group->Attribute("name");
    std::cout << group_name << std::endl;
    if (strncmp(group_name, "AABB", 4) == 0) {
        load_aabb_colliders(stage, obj_group, mem);
    } else if (strncmp(group_name, "TRI", 4) == 0) {
        load_tri_colliders(stage, obj_group, mem);
    }

    world_chunk->active_stage = stage;
}

WorldChunk*
load_world_chunk(mem::GameMem& mem, const char* chunk_name)
{
    WorldChunk* result = mem.game_state_arena.alloc_simple<WorldChunk>();
    load_level_data(mem, result, chunk_name);
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

} // namespace rigel
#if 0

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

auto
parse_tile_csv(const char* csv, TileMap& map)
{
    std::string line;
    std::size_t rows = 0;
    std::size_t cols = 0;
    int map_size = map.width * map.height;

    TileType* tiles = new TileType[map_size];
    uint16_t* sprites = new uint16_t[map_size];
    // glm::mat4* transforms = new glm::mat4[MAP_SIZE];

    std::istringstream csvin(csv);
    while (std::getline(csvin, line)) {
        if ((rows * map.width) + cols >= (map.width * map.height)) {
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
                int idx = (rows * map.width) + cols;
                sprites[idx] = tileno;
                tiles[idx] = (tileno == 0) ? TileType::EMPTY : TileType::WALL;
                // glm::mat4 transform;
                // transforms[idx] = glm::translate(transform, glm::vec3(rows *
                // 16, cols * 16, 0));
            } else {
                std::cerr << "WARN: unexpected value in tile map csv"
                          << std::endl;
            }
            cols++;
        }

        cols = 0;
        rows++;
    }
    map.tiles = tiles;
    map.sprites = sprites;
}

void
dump_tile_map(const TileMap& tilemap)
{
    std::cout << tilemap.width << "x" << tilemap.height << std::endl;

    for (std::size_t row = 0; row < tilemap.height; row++) {
        for (std::size_t col = 0; col < tilemap.width; col++) {
            auto idx = (row * tilemap.width) + col;
            std::cout << tilemap.tiles[idx] << ":" << tilemap.sprites[idx]
                      << " ";
        }
        std::cout << std::endl;
    }
}

auto
load_tile_map_from_xml(const std::filesystem::path& path) -> TileMap
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(path.c_str());

    auto map = doc.FirstChildElement("map");
    if (!map) {
        return {};
    }
    auto width = map->IntAttribute("width");
    auto height = map->IntAttribute("height");

    TileMap tile_map;
    tile_map.width = width;
    tile_map.height = height;

    auto data =
      map->FirstChildElement("layer")->FirstChildElement("data")->GetText();
    parse_tile_csv(data, tile_map);

    return tile_map;
}
#endif
