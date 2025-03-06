#ifndef RIGEL_WORLD_H_
#define RIGEL_WORLD_H_

#include "mem.h"
#include "rigel.h"
#include "entity.h"
#include "resource.h"

#include <iostream>

namespace rigel {

#define WORLD_WIDTH_TILES 40
#define WORLD_HEIGHT_TILES 23
#define WORLD_SIZE_TILES (WORLD_WIDTH_TILES * WORLD_HEIGHT_TILES)

enum class TileType
{
    EMPTY, WALL
};

struct TileMap
{
    TileType tiles[WORLD_SIZE_TILES];
    u16 tile_sprites[WORLD_SIZE_TILES];
};
auto
operator<<(std::ostream& os, const TileType& tt) -> std::ostream&;

/*
OLD
auto parse_tile_csv(const char* csv, TileMap& map);
auto load_tile_map_from_xml(const std::filesystem::path& path) -> TileMap;
*/
void dump_tile_map(const TileMap* tilemap);

TileMap*
load_tile_map_from_xml(mem::Arena& arena, TextResource xml_data);

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
    entity::Entity entities[MAX_ENTITIES];


    // REVIEW
    EntityId add_entity(mem::GameMem& mem,
                        entity::EntityType type,
                        SpriteResourceId sprite_id,
                        glm::vec3 initial_position,
                        Rectangle collider);
    // REVIEW
    entity::Entity* add_player(mem::GameMem& mem,
                               SpriteResourceId sprite_id,
                               glm::vec3 initial_position,
                               Rectangle collider);
};

WorldChunk*
load_world_chunk(mem::GameMem& mem);

} // namespace rigel
#endif // RIGEL_WORLD_H_
