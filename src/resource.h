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
#define MAX_ANIM_RESOURCES 128
#define MAX_ANIMATIONS 8

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

struct TextResource {
    ResourceId resource_id;
    usize length;
    const char* text;
};

struct ImageResource {
    ResourceId resource_id;
    m::Vec2 atlas_coords;
    usize width;
    usize height;
    usize channels;
    usize n_frames;

    ubyte* data;
};

struct Frame
{
    m::Vec2 spritesheet_min;
    m::Vec2 spritesheet_max;
    f32 duration_ms;
};

// TODO(spencer): This has to change from a frame number to a frame area
struct Animation
{
    char name[32];
    usize start_frame;
    usize end_frame;
};

struct AnimationResource
{
    ResourceId id;

    u32 n_frames;
    Frame *frames;

    // TODO(spencer): This kinda sucks, but it might be what I want?
    StringKeyedMap<Animation, MAX_ANIMATIONS> animations;
};

// TODO: This needs to be a proper hash map now
struct ResourceLookup {
    i32 next_free_text_id;
    StringKeyedMap<ResourceId, MAX_TEXT_RESOURCES> text_resource_map;
    TextResource text_resources[MAX_TEXT_RESOURCES];

    i32 next_free_image_id;
    StringKeyedMap<ResourceId, MAX_IMAGE_RESOURCES> image_resource_map;
    ImageResource image_resources[MAX_IMAGE_RESOURCES];

    i32 next_free_anim_id;
    StringKeyedMap<ResourceId, MAX_ANIM_RESOURCES> anim_resource_map;
    AnimationResource anim_resources[MAX_ANIM_RESOURCES];

    mem::Arena text_storage;
    mem::Arena image_storage;
    mem::Arena frame_storage;
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

AnimationResource* load_anim_resource(mem::Arena* scratch_arena, const char* file_path);
AnimationResource* get_or_load_anim_resource(mem::Arena* scratch_arena, const char* file_path);
AnimationResource* get_anim_resource(ResourceId id);
AnimationResource* get_anim_resource(const char* key);

} // namespace rigel

#endif // RIGEL_RESOURCE_H
