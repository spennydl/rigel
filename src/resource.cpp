#include "resource.h"
#include "rigel.h"
#include "mem.h"
#include "rigelmath.h"
#include "json.h"

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

    resource_lookup->text_storage = resource_arena.alloc_sub_arena(1024 * ONE_KB);
    resource_lookup->image_storage = resource_arena.alloc_sub_arena(10 * ONE_MB);
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
get_or_load_image_resource(const char* file_path, usize n_frames)
{
    ImageResource check = get_image_resource(file_path);
    if (check.resource_id == RESOURCE_ID_NONE) {
        return load_image_resource(file_path, n_frames);
    }
    return check;
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

// TODO: This isn't really an animation resource anymore, it's a collection
// of animations in a single sprite sheet.
AnimationResource* load_anim_resource(mem::Arena* scratch_arena, const char* file_path)
{
    assert(resource_lookup->next_free_anim_id < MAX_ANIM_RESOURCES && "Tried to alloc too many animations");

    TextResource info = load_text_resource(file_path);
    auto root_v = parse_json_string(scratch_arena, info.text);
    AnimationResource* resource = resource_lookup->anim_resources + resource_lookup->next_free_anim_id;

    resource->id = resource_lookup->next_free_anim_id;
    resource_lookup->next_free_anim_id++;

    auto root = root_v->object;
    auto meta_obj = jsonobj_get(root, "meta", 4)->object;
    auto animations = jsonobj_get(meta_obj, "frameTags", 9);
    auto frames = jsonobj_get(root, "frames", 6)->obj_array;

    auto head = animations->obj_array;
    while (head)
    {
        Animation animation;

        auto name = jsonobj_get(head->obj, "name", 4);
        char* resource_key = reinterpret_cast<char*>(
            resource_lookup->text_storage.alloc_bytes(name->string->end - name->string->start));

        json_str_copy(animation.name, name->string, 32);
        json_str_copy(resource_key, name->string);

        animation.start_frame = jsonobj_get(head->obj, "from", 4)->number->value;
        animation.end_frame = jsonobj_get(head->obj, "to", 2)->number->value + 1; // store half-open

        auto frames_head = frames;
        i32 n = animation.start_frame;
        while (n--)
        {
            frames_head = frames_head->next;
        }
        animation.ms_per_frame = jsonobj_get(frames_head->obj, "duration", 8)->number->value;

        resource->animations.add(resource_key, animation);

        head = head->next;
    }

    auto frames_head = frames;
    while (frames_head)
    {
        resource->n_frames++;
        frames_head = frames_head->next;
    }

    usize key_len = strlen(file_path) + 1;
    char* resource_key = reinterpret_cast<char*>(resource_lookup->text_storage.alloc_bytes(key_len));
    usize k;
    for (k = 0; k < key_len; k++)
    {
        resource_key[k] = file_path[k];
    }
    resource_key[k] = '\0';
    resource_lookup->anim_resource_map.add(resource_key, resource->id);

    return resource;
}

AnimationResource* get_or_load_anim_resource(mem::Arena* scratch_arena, const char* file_path)
{
    AnimationResource* check = get_anim_resource(file_path);
    if (check)
    {
        return check;
    }
    return load_anim_resource(scratch_arena, file_path);
}

AnimationResource* get_anim_resource(ResourceId id)
{
    assert(id < resource_lookup->next_free_anim_id && "Tried to load oob animation");
    return resource_lookup->anim_resources + id;
}

AnimationResource* get_anim_resource(const char* key)
{
    auto id = resource_lookup->anim_resource_map.get(key);
    if (id)
    {
        return resource_lookup->anim_resources + *id;
    }
    return nullptr;
}

} // namespace rigel
