#ifndef RIGEL_RESOURCE_H
#define RIGEL_RESOURCE_H

#include "rigel.h"
#include "mem.h"

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

struct ResourceLookup {
    usize next_free_text_idx;
    TextResource text_resources[MAX_TEXT_RESOURCES];

    usize next_free_image_idx;
    ImageResource image_resources[MAX_IMAGE_RESOURCES];

    mem::Arena text_storage;
    mem::Arena image_storage;
};

void resource_initialize(mem::Arena& resource_arena);

TextResource load_text_resource(const char* file_path);
TextResource get_text_resource(ResourceId id);

// TODO: load n_frames from a file
ImageResource load_image_resource(const char* file_path, usize n_frames = 1);
ImageResource get_image_resource(ResourceId id);

} // namespace rigel

#endif // RIGEL_RESOURCE_H
