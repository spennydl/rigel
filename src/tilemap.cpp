#include "rigel.h"
#include "tilemap.h"
#include "resource.h"
#include "mem.h"

#include <tinyxml2.h>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>

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
parse_tile_csv(const char* csv, TileMap* map)
{
    std::string line;
    std::size_t rows = 0;
    std::size_t cols = 0;
    usize n_nonempty = 0;

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
                if (tileno == 0) {
                    map->tiles[idx] = TileType::EMPTY;
                } else {
                    map->tiles[idx] = TileType::WALL;
                    n_nonempty++;
                }
            } else {
                std::cerr << "WARN: unexpected value in tile map csv"
                          << std::endl;
            }
            cols++;
        }

        cols = 0;
        rows++;
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
    TileMap* decoration = arena.alloc_simple<TileMap>();
    TileMap* background = arena.alloc_simple<TileMap>();
    tile_map->background = background;
    tile_map->decoration = decoration;

    bool fg = false;
    bool bg = false;
    bool dec = false;
    auto map_el = map->FirstChildElement("layer");
    while (!(fg && bg && dec)) {
        if (strncmp(map_el->Attribute("name"), "fg", 2) == 0) {
            auto data = map_el->FirstChildElement("data")->GetText();
            parse_tile_csv(data, tile_map);
            fg = true;
        }
        if (strncmp(map_el->Attribute("name"), "bg", 2) == 0) {
            auto data = map_el->FirstChildElement("data")->GetText();
            parse_tile_csv(data, background);
            bg = true;
        }
        if (strncmp(map_el->Attribute("name"), "decoration", 10) == 0) {
            auto data = map_el->FirstChildElement("data")->GetText();
            parse_tile_csv(data, decoration);
            dec = true;
        }
        map_el = map_el->NextSiblingElement("layer");
    }

    return tile_map;
}

} // namespace rigel
