#ifndef RIGEL_TILEMAP_H
#define RIGEL_TILEMAP_H

#include "mem.h"
#include "resource.h"
#include "rigelmath.h"

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
    ResourceId tile_sheet;
    TileMap* background;
    TileMap* decoration;
};
auto
operator<<(std::ostream& os, const TileType& tt) -> std::ostream&;

void dump_tile_map(const TileMap* tilemap);

inline isize
tile_to_index(usize tile_x, usize tile_y)
{
    return (tile_y * WORLD_WIDTH_TILES) + tile_x;
}

inline isize
tile_to_index(m::Vec2 tile_coord)
{
    return tile_to_index(tile_coord.x, tile_coord.y);
}

inline m::Vec2
world_to_tiles(m::Vec3 world_coord)
{
    isize x = world_coord.x / TILE_WIDTH_PIXELS;
    isize y = WORLD_HEIGHT_TILES - ((isize)world_coord.y / TILE_WIDTH_PIXELS) - 1;
    return {(f32)x, (f32)y};
}

inline m::Vec3
tile_index_to_world(usize idx)
{
    usize x = idx % WORLD_WIDTH_TILES;
    usize y = WORLD_HEIGHT_TILES - (idx / WORLD_WIDTH_TILES) - 1;
    return {(f32)(x * TILE_WIDTH_PIXELS), (f32)(y * TILE_WIDTH_PIXELS), 0.0f};
}

inline m::Vec3
tiles_to_world(usize tile_x, usize tile_y)
{
    usize x = tile_x * 8;
    usize y = (WORLD_HEIGHT_TILES - tile_y - 1) * TILE_WIDTH_PIXELS;
    return {(f32)x, (f32)y, 0.0f};
}

inline m::Vec3
tiles_to_world(m::Vec2 tile_coord)
{
    return tiles_to_world(tile_coord.x, tile_coord.y);

}

void
fill_tilemap_from_array(TileMap* map, f32* array, usize n_elems);
}


#endif // RIGEL_TILEMAP_H
