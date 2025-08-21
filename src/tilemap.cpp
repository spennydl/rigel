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

void
tilemap_set_up_and_buffer(TileMap* map, mem::Arena* temp_arena)
{
    render::set_up_vertex_buffer_for_rectangles(&map->vert_buffer);

    auto tile_rects = mem::make_simple_list<render::RectangleBufferVertex>(map->n_nonempty_tiles, temp_arena);

    auto tile_dim = m::Vec3 { TILE_WIDTH_PIXELS, TILE_WIDTH_PIXELS, 0 };

    for (u32 y = 0; y < WORLD_HEIGHT_TILES; y++)
    {
        for (u32 x = 0; x < WORLD_WIDTH_TILES; x++)
        {
            u32 i = tile_to_index(x, y);
            if (map->tiles[i] == TileType::EMPTY)
            {
                continue;
            }

            auto world_min = tiles_to_world(x, y);
            auto world_max = world_min + tile_dim;

            auto rect = simple_list_append_new(&tile_rects);
            rect->world_min.x = world_min.x;
            rect->world_min.y = world_min.y;
            rect->world_max.x = world_max.x;
            rect->world_max.y = world_max.y;
            rect->color_and_strength = m::Vec4{1, 0, 0, 1};
        }
    }

    render::buffer_rectangles(&map->vert_buffer, tile_rects.items, tile_rects.length, temp_arena);
}

} // namespace rigel
