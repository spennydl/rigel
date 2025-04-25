#include "resource.h"
#include "rigel.h"
#include "mem.h"
#include "rigelmath.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "stb_image.h"

namespace rigel {

static ResourceLookup* resource_lookup;

// TODO: rewrite
std::string
slurp(std::ifstream& in)
{
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

void
resource_initialize(mem::Arena& resource_arena)
{
    resource_lookup = resource_arena.alloc_simple<ResourceLookup>();

    resource_lookup->text_storage = resource_arena.alloc_sub_arena(32 * ONE_KB);
    resource_lookup->image_storage = resource_arena.alloc_sub_arena(7 * ONE_MB);
}

TextResource
load_text_resource(const char* file_path)
{
    // TODO: check if the friggin thing exists ya numpty
    std::ifstream txt_file(file_path);
    auto txt = slurp(txt_file);

    TextResource* new_resource = resource_lookup->text_resources + resource_lookup->next_free_text_id;
    new_resource->resource_id = resource_lookup->next_free_text_id;
    resource_lookup->next_free_text_id += 1;

    // TODO: I really need my own string lib at some point
    // TODO: also this should move to the map
    usize key_len = strlen(file_path) + 1;
    char* text_resource_key = reinterpret_cast<char*>(resource_lookup->text_storage.alloc_bytes(key_len));
    usize k;
    for (k = 0; k < key_len; k++)
    {
        text_resource_key[k] = file_path[k];
    }
    text_resource_key[k] = '\0';

    new_resource->length = txt.length() + 1; // null term'd!
    char* text_resource = reinterpret_cast<char*>(resource_lookup->text_storage.alloc_bytes(new_resource->length));
    usize i;
    for (i = 0; i < new_resource->length; i++) {
        text_resource[i] = txt[i];
    }
    text_resource[i] = '\0';
    new_resource->text = text_resource;

    resource_lookup->text_resource_map.add(text_resource_key, new_resource->resource_id);

    return *new_resource;
}

TextResource
get_text_resource(ResourceId id)
{
    assert(id < resource_lookup->next_free_text_id && "OOB text resource access");
    return resource_lookup->text_resources[id];
}

TextResource
get_text_resource(const char* file_path)
{
    ResourceId* resource_id = resource_lookup->text_resource_map.get(file_path);
    if (resource_id)
    {
        return resource_lookup->text_resources[*resource_id];

    }
    return TextResource { RESOURCE_ID_NONE, 0, "" };
}


ImageResource
load_image_resource(const char* file_path, usize n_frames)
{
    ImageResource* resource = resource_lookup->image_resources + resource_lookup->next_free_image_id;
    resource->resource_id = resource_lookup->next_free_image_id;
    resource_lookup->next_free_image_id += 1;

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
    resource->data = resource_lookup->image_storage.alloc_bytes(n_bytes);
    for (i32 i = 0; i < n_bytes; i++) {
        resource->data[i] = data[i];
    }
    resource->width = w;
    resource->height = h;
    resource->channels = c;
    resource->n_frames = n_frames;

    usize key_len = strlen(file_path) + 1;
    char* resource_key = reinterpret_cast<char*>(resource_lookup->text_storage.alloc_bytes(key_len));
    usize k;
    for (k = 0; k < key_len; k++)
    {
        resource_key[k] = file_path[k];
    }
    resource_key[k] = '\0';

    resource_lookup->image_resource_map.add(resource_key, resource->resource_id);

    return *resource;
}

ImageResource
get_image_resource(ResourceId id)
{
    assert(id < resource_lookup->next_free_image_id && "OOB image resource access");
    return resource_lookup->image_resources[id];
}

ImageResource
get_image_resource(const char* key)
{
    ResourceId* id = resource_lookup->image_resource_map.get(key);
    if (id)
    {
        return resource_lookup->image_resources[*id];
    }
    ImageResource dummy = {0};
    dummy.resource_id = RESOURCE_ID_NONE;
    return dummy;
}

} // namespace rigel
