#include "resource.h"
#include "rigel.h"
#include "mem.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "stb_image.h"

namespace rigel {

// TODO: rewrite
std::string
slurp(std::ifstream& in)
{
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

inline ResourceLookup*
get_resource_lookup(mem::Arena& resource_arena)
{
    return reinterpret_cast<ResourceLookup*>(resource_arena.mem_begin);
}

void
resource_initialize(mem::Arena& resource_arena)
{
    ResourceLookup* lookup = resource_arena.alloc_simple<ResourceLookup>();

    lookup->text_storage = resource_arena.alloc_sub_arena(32 * ONE_KB);
    lookup->image_storage = resource_arena.alloc_sub_arena(1 * ONE_MB);
}

TextResource
load_text_resource(mem::Arena& resource_arena, const char* file_path)
{
    ResourceLookup* lookup = get_resource_lookup(resource_arena);

    // TODO: check if the friggin thing exists ya numpty
    std::ifstream txt_file(file_path);
    auto txt = slurp(txt_file);

    ResourceId new_id = lookup->next_free_text_idx;

    lookup->next_free_text_idx++;
    assert(lookup->next_free_text_idx < MAX_TEXT_RESOURCES && "Allocated too many text resources!");

    TextResource* new_resource = lookup->text_resources + new_id;
    new_resource->resource_id = new_id;
    new_resource->length = txt.length() + 1; // null term'd!

    char* text_resource = reinterpret_cast<char*>(lookup->text_storage.alloc_bytes(new_resource->length));
    usize i;
    for (i = 0; i < new_resource->length; i++) {
        text_resource[i] = txt[i];
    }
    text_resource[i] = '\0';
    new_resource->text = text_resource;

    return *new_resource;
}

TextResource
get_text_resource(mem::Arena& resource_arena, ResourceId id)
{
    ResourceLookup* lookup = get_resource_lookup(resource_arena);

    assert(id < lookup->next_free_text_idx && "OOB text resource access");

    return lookup->text_resources[id];
}


ImageResource
load_image_resource(mem::Arena& resource_arena, const char* file_path)
{
    ResourceLookup* lookup = get_resource_lookup(resource_arena);
    assert(lookup->next_free_image_idx < MAX_IMAGE_RESOURCES && "Alloc'd too many image resources");

    ResourceId new_id = lookup->next_free_image_idx;
    ImageResource* resource = lookup->image_resources + new_id;
    resource->resource_id = new_id;

    lookup->next_free_image_idx = new_id + 1;

    int w, h, c;
    // TODO: This allocs! need to write an adapter or something like that
    auto data = stbi_load(file_path,
                               &w,
                               &h,
                               &c,
                               4);
    assert(data != nullptr && "Could not load an image"); // TODO: this isn't a show-stopper

    i32 n_bytes = w * h * c;
    std::cout << "Loading image of size " << n_bytes << std::endl;

    // TODO: see above.
    resource->data = lookup->image_storage.alloc_bytes(n_bytes);
    for (i32 i = 0; i < n_bytes; i++) {
        resource->data[i] = data[i];
    }
    resource->width = w;
    resource->height = h;
    resource->channels = c;

    return *resource;
}

ImageResource
get_image_resource(mem::Arena& resource_arena, ResourceId id)
{
    ResourceLookup* lookup = get_resource_lookup(resource_arena);

    assert(id < lookup->next_free_image_idx && "OOB image resource access");

    return lookup->image_resources[id];
}

} // namespace rigel
