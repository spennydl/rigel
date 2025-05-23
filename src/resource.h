#ifndef RIGEL_RESOURCE_H
#define RIGEL_RESOURCE_H

#include "rigel.h"
#include "mem.h"
#include "rigelmath.h"

#include <cstring>

namespace rigel {
typedef i32 SpriteResourceId;
typedef i32 ResourceId;
constexpr static i32 RESOURCE_ID_NONE = -1;

#define MAX_TEXT_RESOURCES 128
#define MAX_IMAGE_RESOURCES 128

struct TextResource {
    ResourceId resource_id;
    usize length;
    const char* text;
};

struct ImageResource {
    ResourceId resource_id;
    usize width;
    usize height;
    usize channels;
    usize n_frames;

    ubyte* data;
};

template<typename T, usize max>
struct StringKeyedMap
{
    struct MapEntry
    {
        const char* key;
        T value;
    };
    usize count;
    MapEntry map[max];

    void add(const char* key, T value)
    {
        count++;
        // keep one free so linear probing always terminates
        assert(count < max - 1 && "Overflowed map");
        usize key_len = strlen(key);

        u64 hash = m::dbj2(key, key_len);
        usize idx = hash % max;

        while (map[idx].key)
        {
            idx++;
            if (idx >= max)
            {
                idx = 0;
            }
        }
        map[idx].key = key;
        map[idx].value = value;
    }

    T* get(const char* key)
    {
        usize key_len = strlen(key);

        u64 hash = m::dbj2(key, key_len);
        usize idx = hash % max;

        while (map[idx].key)
        {
            if (strncmp(map[idx].key, key, key_len) == 0)
            {
                return &map[idx].value;
            }

            idx++;
            if (idx >= max)
            {
                idx = 0;
            }
        }
        return nullptr;
    }
};



// TODO: This needs to be a proper hash map now
struct ResourceLookup {
    usize next_free_text_id;
    StringKeyedMap<ResourceId, MAX_TEXT_RESOURCES> text_resource_map;
    TextResource text_resources[MAX_TEXT_RESOURCES];

    usize next_free_image_id;
    StringKeyedMap<ResourceId, MAX_IMAGE_RESOURCES> image_resource_map;
    ImageResource image_resources[MAX_IMAGE_RESOURCES];

    mem::Arena text_storage;
    mem::Arena image_storage;
};

void resource_initialize(mem::Arena& resource_arena);

TextResource load_text_resource(const char* file_path);
TextResource get_text_resource(ResourceId id);
TextResource get_text_resource(const char* key);

// TODO: load n_frames from a file
ImageResource load_image_resource(const char* file_path, usize n_frames = 1);
ImageResource get_or_load_image_resource(const char* file_path, usize n_frames = 1);
ImageResource get_image_resource(ResourceId id);
ImageResource get_image_resource(const char* key);

} // namespace rigel

#endif // RIGEL_RESOURCE_H
