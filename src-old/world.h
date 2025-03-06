#ifndef WORLD_H
#define WORLD_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

#include "entity.h"
#include "mem.h"
#include "collider.h"
#include "rigel.h"
#include "tinyxml2.h"

#if 0
#define MAP_SIZE (30 * 28)

enum class TileType
{
    EMPTY,
    WALL
};
auto operator<<(std::ostream& os, const TileType& tt) -> std::ostream&;

struct Tile
{
    std::size_t sprite_num;
    TileType type;
};

struct TileMap
{
    std::size_t width;
    std::size_t height;
    TileType* tiles;

    // TODO: these are either unnecessary or should be in render
    uint16_t* sprites;
    //glm::mat4* transforms;
};

auto parse_tile_csv(const char* csv, TileMap& map);
void dump_tile_map(const TileMap& tilemap);
auto load_tile_map_from_xml(const std::filesystem::path& path) -> TileMap;


auto load_level_data(const std::filesystem::path& tmx_path) -> LevelData;
#endif

///////////////////////////////////////////////////
// TODO: We need to re-write basically all of this.
// We wanna keep some of the level loading stuff tho.
///////////////////////////////////////////////////
#define MAX_ENTITIES 64

namespace rigel {

typedef std::size_t StageId;

struct Stage
{
    Rectangle dimensions;
    ColliderSet colliders;
};

// TODO: what even is this?
struct StageDescriptor
{
    StageId id;
    Stage* stage;
};

struct EntityHash
{
    EntityId id;
    usize index;
};

// *Persistent* world chunk data
struct WorldChunk
{
    // need the notion of a WorldLocation
    Stage* active_stage;

    usize next_free_entity_idx;
    EntityHash entity_hash[MAX_ENTITIES];
    entity::Entity entities[MAX_ENTITIES];

    EntityId add_entity(mem::GameMem& mem,
                        entity::EntityType type,
                        SpriteResourceId sprite_id,
                        glm::vec3 initial_position,
                        Rectangle collider);

    entity::Entity* add_player(mem::GameMem& mem,
                               SpriteResourceId sprite_id,
                               glm::vec3 initial_position,
                               Rectangle collider);
};

WorldChunk*
load_world_chunk(mem::GameMem& mem, const char* chunk_name);

}

#endif //
