#include "rigel.h"
#include "tilemap.h"
#include "resource.h"
#include "mem.h"

namespace rigel {
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
fill_tilemap_from_array(TileMap* map, f32* array, usize n_elems)
{
    assert(n_elems == (WORLD_WIDTH_TILES * WORLD_HEIGHT_TILES) && "Unexpected number of tiles");

    usize n_nonempty = 0;
    for (usize tile_i = 0; tile_i < n_elems; tile_i++)
    {
        if (array[tile_i] == 0)
        {
            map->tiles[tile_i] = TileType::EMPTY;
        }
        else
        {
            map->tiles[tile_i] = TileType::WALL;
            n_nonempty++;
        }
        map->tile_sprites[tile_i] = array[tile_i];
    }
    map->n_nonempty_tiles = n_nonempty;
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

} // namespace rigel
