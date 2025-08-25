#ifndef RIGEL_RENDER_H
#define RIGEL_RENDER_H

#include "rigel.h"
#include "resource.h"
#include "collider.h"
#include "rigelmath.h"

#include <glad/glad.h>

#include <string>

namespace rigel {

struct GameState;

namespace render {

const f32 RENDER_INTERNAL_WIDTH = 320.0;
const f32 RENDER_INTERNAL_HEIGHT = 180.0;
const f32 RENDER_SCREEN_WIDTH = 1280.0;
const f32 RENDER_SCREEN_HEIGHT = 720.0;

const f32 QUAD_VERTS[] = { 0.0, 0.0, 0.0, 0.0, 1.0,
                           0.0, 1.0, 0.0, 0.0, 0.0,
                           1.0, 0.0, 0.0, 1.0, 1.0,
                           1.0, 1.0, 0.0, 1.0, 0.0 };

const u32 QUAD_IDXS[] = { 0, 1, 2, 1, 2, 3 };


class Viewport
{
  public:
    explicit Viewport()
      : position {0.0f, 4.0f, 0.0f}
      , scale(1.0)
      , width(RENDER_INTERNAL_WIDTH)
      , height(RENDER_INTERNAL_HEIGHT)
    {
    }
    Viewport(m::Vec3 position)
      : position(position)
      , scale(1.0)
      , width(RENDER_INTERNAL_WIDTH)
      , height(RENDER_INTERNAL_HEIGHT)
    {
    }

    void translate(m::Vec3 by);
    void zoom(float factor);
    m::Mat4 get_screen_transform() const;
    m::Vec3 get_position() const noexcept;

  private:
    m::Vec3 position;
    f32 scale;
    f32 width;
    f32 height;
};

struct Shader
{
    GLuint id;
};
void 
shader_load_from_src(Shader* shader, const char* vs_src, const char* fs_src);
void
shader_set_uniform_m4v(Shader* shader, const char* name, m::Mat4 mat);
void
shader_set_uniform_2fv(Shader* shader, const char* name, m::Vec2 vec);
void
shader_set_uniform_1i(Shader* shader, const char* name, i32 value);
bool
check_shader_status(GLuint id, bool prog = false);

void draw_rectangle(Rectangle rect, f32 r, f32 g, f32 b);

struct TextureConfig
{
    u32 width;
    u32 height;
    u32 wrap_s;
    u32 wrap_t;
    u32 min_filter;
    u32 mag_filter;
    u32 internal_format;
    u32 src_format;
    u32 src_data_type;
    void* data;

    TextureConfig()
        : width(0), height(0),
            wrap_s(GL_REPEAT), wrap_t(GL_REPEAT),
            min_filter(GL_NEAREST), mag_filter(GL_NEAREST),
            internal_format(GL_SRGB_ALPHA), src_format(GL_RGBA),
            src_data_type(GL_UNSIGNED_BYTE), data(nullptr)
    {}
};

struct Texture
{
    GLuint id;
    SpriteResourceId ready_idx;
    m::Vec3 dims;
};
Texture make_texture(TextureConfig config);
Texture make_array_texture_from_vstrip(ImageResource image, usize n_images);

// ??
constexpr static usize MAX_SPRITES_ON_SCREEN = 256;
constexpr static usize MAX_ATLAS_SPRITES = 256;

constexpr static usize SPRITE_BYTES_PER_PIXEL = 4;

struct ResourceTextureMapping
{
    ResourceId resource_id;
    i32 texture_idx;
};

// TODO(spencer): this could be fleshed out to a texture cache. Thing is,
// I'm not sure I actually need one. It has been useful to have a lookup
// for textures, though.
struct TextureLookup
{
    usize next_free_texture_idx;
    ResourceTextureMapping map[64];
    Texture textures[64];
};

struct RenderableAssets
{
    TextureLookup* ready_textures;
};

Texture* get_renderable_texture(ResourceId sprite_id);

// TODO(spencer): chopping block
struct GpuQuad
{
    GLuint vao;

    void initialize();
};

struct RenderTarget
{
    i32 w;
    i32 h;
    i32 l;
    GLuint target_framebuf;
    Texture target_texture;
};

RenderTarget* get_default_render_target();
RenderTarget make_render_to_texture_target(i32 w, i32 h);
RenderTarget make_render_to_array_texture_target(i32 w, i32 h, i32 layers, usize format);

// TODO
struct UniformLight
{
    m::Vec4 position;
    m::Vec4 color;
    m::Vec4 data;
};

struct GlobalUniforms
{
    m::Mat4 screen_transform;
    UniformLight point_lights[24];
    UniformLight circle_lights[24];
    m::Vec4 n_lights;
};

enum GameShaders {
    SCREEN_SHADER = 0,
    TILEMAP_DRAW_SHADER,
    ENTITY_DRAW_SHADER,
    TILE_OCCLUDER_SHADER,
    BACKGROUND_GRADIENT_SHADER,
    SIMPLE_RECTANGLE_SHADER,
    SIMPLE_SPRITE_SHADER,
    SIMPLE_QUAD_SHADER,
#ifdef RIGEL_DEBUG
    DEBUG_LINE_SHADER,
#endif
    N_GAME_SHADERS
};

extern Shader game_shaders[N_GAME_SHADERS];

void initialize_renderer(mem::Arena* gfx_arena, f32 fb_width, f32 fb_height);

void begin_render(Viewport& vp, GameState* game_state, f32 fb_width, f32 fb_height);
void render_background();

void begin_render_to_internal_target();
void begin_render_to_target(RenderTarget target);
void end_render_to_target();

void end_render();

RenderTarget internal_target();

// ------------------------------------

#define MAX_SPRITES 128

struct ImageData
{
    m::Vec2 dimensions;
    ubyte* data;
};

typedef i32 SpriteId;

struct Sprite
{
    SpriteId id;
    m::Vec2 atlas_min;
    m::Vec2 atlas_max;
    m::Vec2 dimensions;
    ubyte* data;
};

struct SpriteAtlas
{
    Texture texture;
    b32 needs_rebuffer;
    SpriteId next_free_sprite_id;
    Sprite sprites[MAX_SPRITES];
};

SpriteId
atlas_push_sprite(SpriteAtlas* atlas, u32 width, u32 height, ubyte* data);
void
atlas_rebuffer(SpriteAtlas* atlas, mem::Arena* tmp_arena);
Sprite*
atlas_get_sprite(SpriteAtlas* atlas, SpriteId sprite_id);

SpriteId
default_atlas_push_sprite(u32 width, u32 height, ubyte* data);
void
default_atlas_rebuffer(mem::Arena* tmp_arena);

Texture*
get_default_sprite_atlas_texture();

// TODO(spencer): this name is bad since this type represents both
// a "vertex" and a rectangle
struct RectangleBufferVertex
{
    m::Vec2 world_min;
    m::Vec2 world_max;
    m::Vec4 color_and_strength;
    m::Vec2 atlas_min;
    m::Vec2 atlas_max;
};

struct QuadBufferVertex
{
    m::Vec4 p;
    m::Vec2 uv;
    m::Vec4 color_and_strength;
};

struct VertexBuffer
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    u32 n_elems;
};

void 
set_up_vertex_buffer_for_rectangles(VertexBuffer* buffer);
void 
set_up_vertex_buffer_for_quads(VertexBuffer* buffer);
void
buffer_rectangles(VertexBuffer* buffer, RectangleBufferVertex* rectangles, u32 n_verts, mem::Arena* scratch_arena);


// TODO(spencer): this new strategy means that I need
// two layers to the renderer: one that is the lowest-level
// operations (below), and another that sits on top and
// implements the actual rendering pipeline for the
// game.
//
// I suppose this is the point of all of this work. We're decoupling
// rendering from the game.

struct BatchBuffer
{
    u32 rect_count;
    u32 quad_count;
    u32 items_in_buffer;

    u32 buffer_size;
    u32 buffer_used;
    ubyte* buffer;
};

// next things I need here:
// - arbitrary quad rendering
// - how does the existing tilemap renderer fit?
//   - we need the concept of retained vertex buffers.
//
// I think that's all? With that in place I can rewrite
// the pipeline in terms of the new renderer. Huh.
#define RENDER_ITEM_LIST \
    X(Rectangle) \
    X(Quad) \
    X(ClearBufferCmd) \
    X(SwitchTargetCmd) \
    X(UseShaderCmd) \
    X(AttachTextureCmd) \
    X(DrawVertexBufferCmd) \
    X(Sprite)

enum RenderItemType
{
    RenderItemType_None = 0,
    #define X(ItemName) RenderItemType_##ItemName,
    RENDER_ITEM_LIST
    #undef X
    RenderItemType_NTypes
};

struct Item
{
    RenderItemType type;
};

struct RectangleItem
{
    RenderItemType type;

    m::Vec3 min;
    m::Vec3 max;
    m::Vec4 color_and_strength;
};

struct ClearBufferCmdItem
{
    RenderItemType type;

    m::Vec4 clear_color;
    b32 clear_depth;
};

struct SwitchTargetCmdItem
{
    RenderItemType type;

    RenderTarget* target;
};

struct UseShaderCmdItem
{
    RenderItemType type;

    Shader* shader;
};


struct AttachTextureCmdItem
{
    RenderItemType type;

    u32 slot;
    Texture* texture;
};

struct DrawVertexBufferCmdItem
{
    RenderItemType type;

    VertexBuffer* buffer;
};

struct SpriteItem
{
    RenderItemType type;

    SpriteId sprite_id;
    m::Vec4 color_and_strength;
    m::Vec3 position;
    // TODO(spencer): these should have better names
    m::Vec2 sprite_segment_min;
    m::Vec2 sprite_segment_max;
};

struct QuadItem
{
    RenderItemType type;

    m::Vec4 v1;
    m::Vec4 v2;
    m::Vec4 v3;
    m::Vec4 v4;
    m::Vec4 color_and_strength;
};

inline b32
is_vertex_buffer_renderable(VertexBuffer* bufer)
{
    return bufer->vao > 0;
}

template <typename T>
inline RenderItemType
get_render_item_type_for()
{
    static_assert(false, "T is not a render item");
    return RenderItemType_None;
}

// lmao i heard you like metaprogramming
#define X(ItemName) \
template <> \
inline RenderItemType \
get_render_item_type_for<ItemName##Item>() \
{ \
    return RenderItemType_##ItemName; \
}

RENDER_ITEM_LIST

#undef X

BatchBuffer*
make_batch_buffer(mem::Arena* target_arena, u32 size_in_bytes = 1024);

void
submit_batch(BatchBuffer* batch, mem::Arena* temp_arena);

template<typename T, typename U>
struct is_same
{
    static constexpr i32 value = false;
};

template<typename T>
struct is_same<T, T>
{
    static constexpr i32 value = true;
};

template<typename T>
struct is_rect_like
{
    static constexpr i32 value = false;
};

template<>
struct is_rect_like<RectangleItem>
{
    static constexpr i32 value = true;
};

template<>
struct is_rect_like<SpriteItem>
{
    static constexpr i32 value = true;
};

inline b32
is_renderable(RenderItemType type)
{
    return type == RenderItemType_Rectangle ||
           type == RenderItemType_Sprite    ||
           type == RenderItemType_Quad;
}

template <typename T>
T*
push_render_item(BatchBuffer* buffer)
{
    assert(buffer->buffer_used + sizeof(T) < buffer->buffer_size);

    auto ptr = buffer->buffer + buffer->buffer_used;
    T* result = new (ptr) T();
    result->type = get_render_item_type_for<T>();
    assert(result->type != RenderItemType_None && "huh");

    buffer->buffer_used += sizeof(T);
    buffer->items_in_buffer += 1;

    if constexpr (is_rect_like<T>::value)
    {
        buffer->rect_count += 1;
    }
    else if constexpr (is_same<T, QuadItem>::value)
    {
        buffer->quad_count += 1;
    }

    return result;
}

inline RectangleItem*
batch_push_rectangle(BatchBuffer* batch, m::Vec3 min, m::Vec3 max, m::Vec4 color_and_strength)
{
    auto item = push_render_item<RectangleItem>(batch);
    item->min = min;
    item->max = max;
    item->color_and_strength = color_and_strength;
    return item;
}

inline ClearBufferCmdItem*
batch_push_clear_buffer_cmd(BatchBuffer* batch, m::Vec4 clear_color, b32 include_depth_buffer)
{
    auto item = push_render_item<ClearBufferCmdItem>(batch);
    item->clear_color = clear_color;
    item->clear_depth = include_depth_buffer;
    return item;
}

inline SwitchTargetCmdItem*
batch_push_switch_target_cmd(BatchBuffer* batch, RenderTarget* target)
{
    auto item = push_render_item<SwitchTargetCmdItem>(batch);
    item->target = target;
    return item;
}

inline UseShaderCmdItem*
batch_push_use_shader_cmd(BatchBuffer* batch, Shader* shader)
{
    auto item = push_render_item<UseShaderCmdItem>(batch);
    item->shader = shader;
    return item;
}

inline AttachTextureCmdItem*
batch_push_attach_texture_cmd(BatchBuffer* batch, u32 slot, Texture* texture)
{
    auto item = push_render_item<AttachTextureCmdItem>(batch);
    item->slot = slot;
    item->texture = texture;
    return item;
}

inline DrawVertexBufferCmdItem*
batch_push_draw_vertex_buffer_cmd(BatchBuffer* batch, VertexBuffer* buffer)
{
    auto item = push_render_item<DrawVertexBufferCmdItem>(batch);
    item->buffer = buffer;
    return item;
}

inline SpriteItem*
batch_push_sprite(BatchBuffer* batch, 
                       SpriteId sprite_id, 
                       m::Vec4 color_and_strength, 
                       m::Vec3 position,
                       m::Vec2 sprite_segment_min,
                       m::Vec2 sprite_segment_max)
{
    auto item = push_render_item<SpriteItem>(batch);
    item->sprite_id = sprite_id;
    item->color_and_strength = color_and_strength;
    item->position = position;
    item->sprite_segment_min = sprite_segment_min;
    item->sprite_segment_max = sprite_segment_max;
    return item;
}

inline QuadItem*
batch_push_quad(BatchBuffer* batch,
                m::Vec4 v1,
                m::Vec4 v2,
                m::Vec4 v3,
                m::Vec4 v4,
                m::Vec4 color_and_strength)
{
    auto item = push_render_item<QuadItem>(batch);
    item->v1 = v1;
    item->v2 = v2;
    item->v3 = v3;
    item->v4 = v4;
    item->color_and_strength = color_and_strength;
    return item;
}


} // namespace render

} // namespace rigel


#endif // RIGEL_RENDER_H
