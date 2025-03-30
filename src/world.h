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
    usize n_nonempty_tiles;
    TileType tiles[WORLD_SIZE_TILES];
    u16 tile_sprites[WORLD_SIZE_TILES];
    TileMap* background;
    TileMap* decoration;
};
auto
operator<<(std::ostream& os, const TileType& tt) -> std::ostream&;

/*
OLD
auto parse_tile_csv(const char* csv, TileMap& map);
auto load_tile_map_from_xml(const std::filesystem::path& path) -> TileMap;
*/
void dump_tile_map(const TileMap* tilemap);

inline usize
tile_to_index(usize tile_x, usize tile_y)
{
    return (tile_y * WORLD_WIDTH_TILES) + tile_x;
}

inline glm::vec2
world_to_tiles(glm::vec3 world_coord)
{
    isize x = (isize)world_coord.x / 8;
    isize y = WORLD_HEIGHT_TILES - ((isize)world_coord.y / 8) - 1;
    assert(y >= 0 && "huh it happened");
    return glm::vec2(x, y);
}

inline glm::vec3
tiles_to_world(usize tile_x, usize tile_y)
{
    usize x = tile_x * 8;
    usize y = (WORLD_HEIGHT_TILES - tile_y - 1) * 8;
    return glm::vec3(x, y, 0);
}

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
