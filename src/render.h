#ifndef RIGEL_RENDER_H
#define RIGEL_RENDER_H

#include "rigel.h"
#include "resource.h"
#include "collider.h"
#include "world.h"
#include "game.h"
#include "rigelmath.h"

#include <glad/glad.h>

#include <string>

namespace rigel {

namespace render {

const f32 RENDER_INTERNAL_WIDTH = 320.0;
const f32 RENDER_INTERNAL_HEIGHT = 180.0;
const f32 RENDER_SCREEN_WIDTH = 1280.0;
const f32 RENDER_SCREEN_HEIGHT = 720.0;

const f32 QUAD_VERTS[] = { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0,
                       1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0 };

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

    Shader();
    Shader(const char* vs_src, const char* fs_src);

    void load_from_src(const char* vs_src, const char* fs_src);
};
bool
check_shader_status(GLuint id, bool prog = false);

void draw_rectangle(Rectangle rect, f32 r, f32 g, f32 b);

struct Texture
{
    GLuint id;
    SpriteResourceId ready_idx;
};
Texture alloc_texture(int w, int h);
Texture make_texture(int w, int h, ubyte* data);
Texture make_texture(ImageResource image);
Texture make_array_texture_from_vstrip(ImageResource image, usize n_images);

struct Quad
{
    rigel::Rectangle dims;
    Shader shader;
    GLuint vao;

    // Quad(Rectangle rect);
    Quad(rigel::Rectangle rect, Shader shader);
};

struct Tri
{
    GLuint vao;
    m::Vec3 verts[3];
    Shader shader;

    // Quad(Rectangle rect);
    Tri(m::Vec3 v1, m::Vec3 v2, m::Vec3 v3, Shader shader);
};

void
render_quad(Quad& quad, Viewport& viewport, int r, int g, int b);
void
render_tri(Tri& quad, Viewport& viewport, int r, int g, int b);

struct BatchedTileRenderer {
    GLuint vao;
    usize n_tiles;
    ResourceId tile_sheet;

    // TODO: Tilemaps will never change. Why store them like I do? Why not store the geometry
    // we make here instead?
    //
    // On second thought we absolutely need them in a grid format for spatial queries. Still,
    // might be nice to keep some around.
    BatchedTileRenderer(mem::Arena& scratch_arena, TileMap* tilemap, ImageResource atlas_image);
    BatchedTileRenderer() : vao(0), n_tiles(0), tile_sheet(0) {}

    void render(Viewport& viewport, Shader shader);
};

constexpr static usize MAX_SPRITES_ON_SCREEN = 256;
constexpr static usize MAX_ATLAS_SPRITES = 256;

constexpr static usize SPRITE_BYTES_PER_PIXEL = 4;

struct ResourceTextureMapping
{
    ResourceId resource_id;
    i32 texture_idx;
};

struct TextureLookup
{
    usize next_free_texture_idx;
    ResourceTextureMapping map[64];
    Texture textures[64];
};

struct ShaderLookup
{
    usize next_free_shader_idx;
    Shader shaders[64];
};

struct WorldChunkDrawData
{
    i32 renderable;
    BatchedTileRenderer fg_renderer;
    BatchedTileRenderer bg_renderer;
    BatchedTileRenderer deco_renderer;
};
void make_world_chunk_renderable(mem::Arena* scratch_mem, WorldChunk* world_chunk);

struct RenderableAssets
{
    ShaderLookup* ready_shaders;
    TextureLookup* ready_textures;
    WorldChunkDrawData* renderable_world_maps;
};

Shader* get_renderable_shader(TextResource vs_src, TextResource fs_src);
Texture* get_renderable_texture(ResourceId sprite_id);

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

RenderTarget create_new_render_target();

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
};

enum GameShaders {
    SCREEN_SHADER = 0,
    TILEMAP_DRAW_SHADER,
    ENTITY_DRAW_SHADER,
    TILE_OCCLUDER_SHADER,
#ifdef RIGEL_DEBUG
    DEBUG_LINE_SHADER,
#endif
    N_GAME_SHADERS
};

void initialize_renderer(mem::Arena* gfx_arena, f32 fb_width, f32 fb_height);

void begin_render(Viewport& vp, GameState* game_state, f32 fb_width, f32 fb_height);
void lighting_pass(mem::Arena* scratch_arena, TileMap* tile_map);
void render_foreground_layer(Viewport& viewport, WorldChunk* world_chunk);
void render_background_layer(Viewport& viewport, WorldChunk* world_chunk);
void render_decoration_layer(Viewport& viewport, WorldChunk* world_chunk);
void make_shadow_map_for_point_light(mem::Arena* scratch_arena, TileMap* tile_map, m::Vec3 light_pos, i32 light_idx);
void render_all_entities(Viewport& viewport, WorldChunk* world_chunk);

void begin_render_to_internal_target();
void begin_render_to_target(RenderTarget target);
void end_render_to_target();

void end_render();

RenderTarget internal_target();

} // namespace render

} // namespace rigel


#endif // RIGEL_RENDER_H
