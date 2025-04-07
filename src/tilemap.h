#ifndef RIGEL_TILEMAP_H
#define RIGEL_TILEMAP_H

#include "mem.h"
#include "resource.h"

#include <tinyxml2.h>
#include <iostream>
#include <glm/glm.hpp>

namespace rigel {

#define TILE_WIDTH_PIXELS 8
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

void dump_tile_map(const TileMap* tilemap);

inline usize
tile_to_index(usize tile_x, usize tile_y)
{
    return (tile_y * WORLD_WIDTH_TILES) + tile_x;
}

inline glm::vec2
world_to_tiles(glm::vec3 world_coord)
{
    isize x = (isize)world_coord.x / TILE_WIDTH_PIXELS;
    isize y = WORLD_HEIGHT_TILES - ((isize)world_coord.y / TILE_WIDTH_PIXELS) - 1;
    assert(y >= 0 && "huh it happened");
    return glm::vec2(x, y);
}

inline glm::vec3
tiles_to_world(usize tile_x, usize tile_y)
{
    usize x = tile_x * 8;
    usize y = (WORLD_HEIGHT_TILES - tile_y - 1) * TILE_WIDTH_PIXELS;
    return glm::vec3(x, y, 0);
}

TileMap*
load_tile_map_from_xml(mem::Arena& arena, TextResource xml_data);
}


#endif // RIGEL_TILEMAP_H
