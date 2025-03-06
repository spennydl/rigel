#ifndef RENDER_H_
#define RENDER_H_

#include "glad/gl.h"
#include "collider.h"
#include "resource.h"
#include "rigel.h"

namespace rigel {

struct WorldChunk;

namespace render {

const f32 RENDER_INTERNAL_WIDTH = 320.0;
const f32 RENDER_INTERNAL_HEIGHT = 180.0;

const f32 QUAD_VERTS[] = { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0,
                       1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0 };

const u32 QUAD_IDXS[] = { 0, 1, 2, 1, 2, 3 };

class Viewport
{
  public:
    explicit Viewport()
      : position(0, 0, 0)
      , scale(1.0)
      , width(RENDER_INTERNAL_WIDTH)
      , height(RENDER_INTERNAL_HEIGHT)
    {
    }
    Viewport(glm::vec3 position)
      : position(position)
      , scale(1.0)
      , width(RENDER_INTERNAL_WIDTH)
      , height(RENDER_INTERNAL_HEIGHT)
    {
    }

    void translate(glm::vec3 by);
    void zoom(float factor);
    glm::mat4 get_screen_transform() const;
    glm::vec3 get_position() const noexcept;

  private:
    glm::vec3 position;
    float scale;
    f32 width;
    f32 height;
};

struct Shader
{
    GLuint id;
    // RESOURCE
    Shader(const std::string& vs_src, const std::string& fs_src);
    Shader(const char* vs_src, const char* fs_src);
    Shader();

    void load_from_src(const std::string& vs_src, const std::string& fs_src);
    void load_from_src(const char* vs_src, const char* fs_src);
};
bool
check_shader_status(GLuint id, bool prog = false);

#if 0
// TODO: Probly want sprites in their own file/layer
struct SpriteAtlas
{
    GLuint id;
    int tiles;
    SpriteAtlas(ImageResource data);
};
// RESOURCE
unsigned char*
unpack_sprite_atlas(ImageResource image,
                    i32* out_levels);

struct TileMap;

struct MapDrawLayer
{
    GLuint vao;
    GLuint ebo;
    GLuint map_buf;
    GLuint map_buf_tex;

    SpriteAtlas atlas;
    Shader shader_prog;

    MapDrawLayer(SpriteAtlas atlas, Shader prog);

    void buffer_map(const TileMap& map);
};

void
render_map(const MapDrawLayer& map);
#endif

struct Texture
{
    GLuint id;
    Texture() : id(0) {}

    // RESOURCE
    Texture(const std::string& path);
    Texture(int w, int h);
    Texture(int w, int h, ubyte* data);
};

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
    glm::vec3 verts[3];
    Shader shader;

    // Quad(Rectangle rect);
    Tri(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, Shader shader);
};

struct RenderLine
{
    glm::vec3 start;
    glm::vec3 start_color;
    glm::vec3 end;
    glm::vec3 end_color;

    RenderLine()
      : start(0)
      , start_color(0)
      , end(0)
      , end_color(0)
    {
    }
};

class VectorRenderer
{
  public:
    VectorRenderer();
    int add_line(RenderLine line);
    void update_line(int idx, RenderLine line);
    void render(Viewport& viewport);

  private:
    GLuint vao;
    GLuint vbo;
    Shader line_shader;
    std::vector<RenderLine> lines;
    bool needs_rebuffer;
};

void
render_quad(Quad& quad, Viewport& viewport, int r, int g, int b);

void
render_tri(Tri& quad, Viewport& viewport, int r, int g, int b);

//////////////////////////////////////////////////
// TODO: we kinda gotta start from scratch here
//////////////////////////////////////////////////

/*
 *
 * I kinda need some sorta scene management, no?
 * At the very least we need to sort by z level.
 *
 * I want to have sprites stored in high->low z order,
 * enable depth testing and render them instanced. I think.
 *
 */

enum RenderStepType
{
    RENDER_STEP_SPRITES,
    RENDER_STEP_STAGE
};

// what are we gonna need for sprites?
// - atlas on the gpu -> all entities in the stage should load on stage load?
// - a list of sprites to render
//
// resources will have a sprite resource ID, need to map to a sprite ID
// to use on the GPU

constexpr static usize MAX_SPRITES_ON_SCREEN = 256;
constexpr static usize MAX_ATLAS_SPRITES = 256;

constexpr static usize SPRITE_BYTES_PER_PIXEL = 4;

// TODO: We're using this kinda backwards.
// We need to hash the resource id and get the sprite idx from it
struct SpriteMapId
{
    i32 idx;
    SpriteResourceId resource_id;
};

struct Sprite
{
    SpriteResourceId id;
    GLuint tex_id;
    usize width;
    usize height;
};

struct SpriteAtlas
{
    usize next_free_idx;
    SpriteMapId loaded_sprites[MAX_ATLAS_SPRITES];
    Sprite sprites[MAX_ATLAS_SPRITES];

    SpriteAtlas();

    // TODO: why return SpriteId?
    SpriteResourceId load_sprite(SpriteResourceId resource_id,
                                 usize width,
                                 usize height,
                                 ubyte* data);

    Sprite* lookup(SpriteResourceId id);
};

struct GpuQuad
{
    GLuint vao;

    void initialize();
};

struct RenderTarget
{
    GLuint target_framebuf;
    int w;
    int h;
};

RenderTarget create_new_render_target();

struct SpriteRenderStep
{
    Shader sprite_shader;
    GpuQuad quad;
    SpriteAtlas* atlas;

    void render_entities_in_world(Viewport* viewport, WorldChunk* world_chunk);
};

struct StageRenderStep
{
    Shader shader;
    GpuQuad quad;
    Texture texture;
};

struct RenderStep
{
    RenderStepType type;
    union
    {
        SpriteRenderStep* sprite_step;
        StageRenderStep* stage_step;
    };
};

struct GlobalUniforms
{
    glm::vec3 viewport_world_pos;
};
static GlobalUniforms GLOBAL_UNIFORMS;

struct RenderState
{
    GLuint global_ubo;
    RenderTarget internal_target;
    Texture texture;
    GpuQuad screen;
    Shader screen_shader;
    f32 fb_width;
    f32 fb_height;
};

void initialize_renderer(f32 fb_width, f32 fb_height);

void begin_render(f32 fb_width, f32 fb_height);

void begin_render_to_target(RenderTarget target);
void end_render_to_target();

void render_stage_background_layer(Viewport& viewport, Texture& texture, Shader& shader);

void end_render();

RenderTarget internal_target();


} // rigel
} // render

#endif // RENDER_H_
