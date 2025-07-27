#include "render.h"
#include "world.h"
#include "resource.h"
#include "rigelmath.h"
#include "debug.h"
#include <glad/glad.h>

// TODO: remove
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace rigel {
namespace render {

static Shader game_shaders[N_GAME_SHADERS];

struct RenderState
{
    mem::Arena* gfx_arena;

    RenderTarget internal_target;
    RenderTarget screen_target;
    RenderTarget shadowmap_target;
    GpuQuad screen;

    Rectangle current_viewport;
    // TODO: just a test
    u32 bg_image_id;

    // TODO: should this be here?
    GLuint global_ubo;
    GlobalUniforms global_uniforms;
};


static RenderState render_state;

#ifdef RIGEL_DEBUG
struct DebugRenderState
{
    GLuint lines_vao;
    GLuint lines_vbo;
};
static DebugRenderState debug_state;
#endif

bool
check_shader_status(GLuint id, bool prog)
{
    int success;
    char infoLog[512];
    if (prog) {
        glGetProgramiv(id, GL_LINK_STATUS, &success);
    } else {
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    }
    if (!success) {
        if (prog) {
            glGetProgramInfoLog(id, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::LINK_FAIL\n" << infoLog << std::endl;
        } else {
            glGetShaderInfoLog(id, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::COMP_FAILED\n" << infoLog << std::endl;
        }
    }
    return success;
}

Shader::Shader() {}

void
Shader::load_from_src(const char* vs_src, const char* fs_src)
{
    GLuint vs, fs;
    vs = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vs, 1, &vs_src, nullptr);
    glCompileShader(vs);
    if (!check_shader_status(vs)) {
        std::cerr << "vertex failed" << std::endl;
        return;
    }

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, nullptr);
    glCompileShader(fs);
    if (!check_shader_status(fs)) {
        std::cerr << "frag failed" << std::endl;
        return;
    }

    this->id = glCreateProgram();
    glAttachShader(this->id, vs);
    glAttachShader(this->id, fs);
    glLinkProgram(this->id);
    if (!check_shader_status(this->id, true)) {
        std::cerr << "link failed" << std::endl;
    }

    u32 uniform_block = glGetUniformBlockIndex(this->id, "GlobalUniforms");
    glUniformBlockBinding(this->id, uniform_block, 0);
}

Shader::Shader(const char* vs_src, const char* fs_src)
{
    load_from_src(vs_src, fs_src);
}

// TODO: Batching!
// We need to generate a single VBO containing all of the tiles to render.
// Might have to go back to using a single atlas image instead of a 2D array
// texture?

// TODO: Tilemaps will never change. Why store them like I do? Why not store the geometry
// we make here instead?
//
// On second thought we absolutely need them in a grid format for spatial queries. Still,
// might be nice to keep some around.
BatchedTileRenderer::BatchedTileRenderer(mem::Arena& scratch_arena, TileMap* tilemap, ImageResource atlas_image)
    : vao(0), n_tiles(tilemap->n_nonempty_tiles)
{
    usize quad_elems = 20;
    usize idx_elems = 6;

    scratch_arena.reinit();
    f32* verts = scratch_arena.alloc_array<f32>(quad_elems * n_tiles);
    u32* indices = scratch_arena.alloc_array<usize>(idx_elems * n_tiles);

    usize tiles_per_row = atlas_image.width / 8;
    usize tiles_per_col = atlas_image.height / 8;

    f32 u_step = 1.0 / (f32)tiles_per_row;
    f32 v_step = 1.0 / (f32)tiles_per_col;

    usize internal_idx = 0;
    for (usize y = 0; y < WORLD_HEIGHT_TILES; y++) {
        for (usize x = 0; x < WORLD_WIDTH_TILES; x++) {
            usize i = (y * WORLD_WIDTH_TILES) + x;
            if (tilemap->tiles[i] == TileType::EMPTY) {
                continue;
            }

            usize tile_sprite = tilemap->tile_sprites[i] - 1;

            f32 u = (tile_sprite % tiles_per_row) * u_step;
            f32 v = (tile_sprite / tiles_per_row) * v_step;

            usize quad_start = internal_idx * quad_elems;
            usize idx_start = internal_idx * idx_elems;

            verts[quad_start] = x;
            verts[quad_start + 1] = y;
            verts[quad_start + 2] = 0.0;
            verts[quad_start + 3] = u;
            verts[quad_start + 4] = v;

            verts[quad_start + 5] = x;
            verts[quad_start + 6] = y + 1;
            verts[quad_start + 7] = 0.0;
            verts[quad_start + 8] = u;
            verts[quad_start + 9] = v + v_step;

            verts[quad_start + 10] = x + 1;
            verts[quad_start + 11] = y;
            verts[quad_start + 12] = 0.0;
            verts[quad_start + 13] = u + u_step;
            verts[quad_start + 14] = v;

            verts[quad_start + 15] = x + 1;
            verts[quad_start + 16] = y + 1;
            verts[quad_start + 17] = 0.0;
            verts[quad_start + 18] = u + u_step;
            verts[quad_start + 19] = v + v_step;

            usize elem_idx_start = quad_start / 5;
            indices[idx_start]     = elem_idx_start + 0;
            indices[idx_start + 1] = elem_idx_start + 1;
            indices[idx_start + 2] = elem_idx_start + 2;
            indices[idx_start + 3] = elem_idx_start + 1;
            indices[idx_start + 4] = elem_idx_start + 2;
            indices[idx_start + 5] = elem_idx_start + 3;

            internal_idx += 1;
        }
    }
    std::cout << "There were " << n_tiles << " tiles and we found " << internal_idx << " of them" << std::endl;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, n_tiles * quad_elems * sizeof(f32), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_tiles * idx_elems * sizeof(u32), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    tile_sheet = atlas_image.resource_id;

    scratch_arena.reinit();
}

// TODO:
// I have yet to come up with a consistent "render" function interface.
// Do I pass shaders? Or do I have some immediate-mode api thru which I can
// use and set uniforms? Or option C? Dunno what is better.
//
// For now I'm doing this
void BatchedTileRenderer::render(Viewport& viewport, Shader shader)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m::Mat4 world_transform =
        m::translation_by(m::Vec3 {0.0f, -WORLD_HEIGHT_TILES, 0.0f}) *
        m::scale_by(m::Vec3 {8.0f, -8.0f, 0.0f});

    Texture* tex = get_renderable_texture(tile_sheet);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, render_state.shadowmap_target.target_texture.id);

    glUseProgram(shader.id);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "world"), 1, GL_FALSE, reinterpret_cast<f32*>(&world_transform));
    glUniform1i(glGetUniformLocation(shader.id, "atlas"), 0);
    glUniform1i(glGetUniformLocation(shader.id, "shadow_map"), 1);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6 * n_tiles, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

Texture make_texture(TextureConfig config)
{
    Texture tex;

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, config.wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, config.wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, config.min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, config.mag_filter);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 config.internal_format,
                 config.width,
                 config.height,
                 0,
                 config.src_format,
                 config.src_data_type,
                 config.data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

Quad::Quad(rigel::Rectangle rect, Shader shader)
  : dims(rect)
  , shader(shader)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_IDXS), QUAD_IDXS, GL_STATIC_DRAW);

    glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Texture make_array_texture_from_vstrip(ImageResource image, TextureConfig config)
{
    Texture tex;
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.id);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, config.wrap_s);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, config.wrap_t);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, config.min_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, config.mag_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);

    usize slice_height = image.height / image.n_frames;

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 config.internal_format,
                 image.width,
                 slice_height,
                 image.n_frames,
                 0,
                 config.src_format,
                 config.src_data_type,
                 image.data);

    return tex;
}

Texture alloc_array_texture(usize w, usize h, usize layers, usize internal_format)
{
    Texture tex;
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.id);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 internal_format,
                 w, h, layers,
                 0,
                 internal_format, GL_UNSIGNED_BYTE, nullptr);
    return tex;
}

void
render_quad(Quad& quad, Viewport& vp, int r, int g, int b)
{
    auto screen_transform = vp.get_screen_transform();
    auto quad_world = quad.dims.get_world_transform();

    glUseProgram(quad.shader.id);
    glUniformMatrix4fv(glGetUniformLocation(quad.shader.id, "screen"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&screen_transform));
    glUniformMatrix4fv(glGetUniformLocation(quad.shader.id, "world"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&quad_world));
    glUniform4f(glGetUniformLocation(quad.shader.id, "color"),
                (float)r / 255.0,
                (float)g / 255.0,
                (float)b / 255.0,
                1.0);

    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Tri::Tri(m::Vec3 v1, m::Vec3 v2, m::Vec3 v3, Shader shader)
  : shader(shader)
{

    verts[0] = v1;
    verts[1] = v2;
    verts[2] = v3;
    usize idxs[3] = { 0, 1, 2 };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

    glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void
render_tri(Tri& tri, Viewport& vp, int r, int g, int b)
{
    auto screen_transform = vp.get_screen_transform();

    glUseProgram(tri.shader.id);
    glUniformMatrix4fv(glGetUniformLocation(tri.shader.id, "screen"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&screen_transform));
    glUniform4f(glGetUniformLocation(tri.shader.id, "color"),
                (float)r / 255.0,
                (float)g / 255.0,
                (float)b / 255.0,
                1.0);

    glBindVertexArray(tri.vao);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void
Viewport::translate(m::Vec3 by)
{
    this->position = this->position + by;
}

m::Vec3
Viewport::get_position() const noexcept
{
    return this->position;
}

m::Mat4
Viewport::get_screen_transform() const
{
    return
           m::scale_by({this->scale * (2.0f / this->width),
                        this->scale * (2.0f / this->height), 0.0f}) *
           m::translation_by({-1.0f, -1.0f, 0.0f});
}

void
Viewport::zoom(float factor)
{
    this->scale = factor;
}

void GpuQuad::initialize()
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_IDXS), QUAD_IDXS, GL_STATIC_DRAW);

    glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


Shader* get_renderable_shader(TextResource vs_src, TextResource fs_src)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    ShaderLookup* ready_shaders = assets->ready_shaders;

    usize next_idx = ready_shaders->next_free_shader_idx;
    assert(next_idx < 64 && "Too many shaders at once");
    Shader* result = ready_shaders->shaders + next_idx;
    result->load_from_src(vs_src.text, fs_src.text);

    return result;
}

// TODO: can have a global RenderableAssets pointer since
// gfx arena will never move.
Texture* get_renderable_texture(ResourceId sprite_id)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    TextureLookup* ready_textures = assets->ready_textures;

    auto map_idx = sprite_id & 63;
    auto map_entry = ready_textures->map + map_idx;
    if (map_entry->resource_id == sprite_id) {
        return ready_textures->textures + map_entry->texture_idx;
    }

    if (map_entry->resource_id < 0) {
        usize next_idx = ready_textures->next_free_texture_idx;
        assert(next_idx < 64 && "Too many textures at once");

        Texture* result = ready_textures->textures + next_idx;

        ImageResource image_resource = get_image_resource(sprite_id);
        TextureConfig tex_config;
        tex_config.width = image_resource.width;
        tex_config.height = image_resource.height;
        tex_config.data = image_resource.data;
        if (image_resource.n_frames == 1) {
            *result = make_texture(tex_config);
        } else {
            *result = make_array_texture_from_vstrip(image_resource, tex_config);
        }

        map_entry->resource_id = map_idx;
        map_entry->texture_idx = next_idx;

        ready_textures->next_free_texture_idx++;

        return result;
    }

    assert(false && "out of map entries");
}

void make_world_chunk_renderable(mem::Arena* scratch_arena, WorldChunk* world_chunk)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_maps + world_chunk->level_index;

    if (draw_data->renderable) {
        return;
    }

    TileMap* map = world_chunk->active_map;
    ImageResource fg_tile_set = get_image_resource(map->tile_sheet);
    ImageResource bg_tile_set = get_image_resource(map->background->tile_sheet);
    ImageResource deco_tile_set = get_image_resource(map->decoration->tile_sheet);

    draw_data->fg_renderer = BatchedTileRenderer(*scratch_arena, map, fg_tile_set);
    draw_data->bg_renderer = BatchedTileRenderer(*scratch_arena, map->background, bg_tile_set);
    draw_data->deco_renderer = BatchedTileRenderer(*scratch_arena, map->decoration, deco_tile_set);

    draw_data->renderable = 1;
}

const char* screen_vs_src = R"SRC(
#version 400 core
layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 uv;
out vec2 tex_uv;
uniform mat4 screen_transform;
uniform mat4 world_transform;

void main() {
    gl_Position = screen_transform * world_transform * vec4(vert, 1.0);
    tex_uv = uv;
})SRC";

const char* screen_fs_src = R"SRC(
#version 400 core
in vec2 tex_uv;
uniform sampler2D game;
out vec4 FragColor;
float dist[6] = float[](0.92, 0.96, 1.0, 1.0, 0.96, 0.92);

void main(){
    float y_pos = (tex_uv.y * 1080.0f);
    int idx = int(y_pos) % 6;
    float mult = dist[idx];

    float x_pos = (tex_uv.x * 1920.0);
    vec2 left = vec2((x_pos - 1) / 1920.0, y_pos / 1080.0);
    vec2 right = vec2((x_pos + 1) / 1920.0, y_pos / 1080.0);

    vec4 left_color = texture(game, left);
    vec4 center = texture(game, tex_uv);
    vec4 right_color = texture(game, right);

    vec4 final = mult * vec4(left_color.r, center.g, right_color.b, center.a);

    //float exposure = 0.3;
    //final.rgb = vec3(1.0) - exp(-final.rgb * exposure);
    final.rgb = pow(final.rgb, vec3(1.0/2.2));
    FragColor = final;
})SRC";

const char* fs_gradient = R"SRC(
#version 400 core
in vec2 tex_uv;
out vec4 FragColor;

uniform sampler2D bg_tex;

mat4 dither = (1.0 / 16.0) * mat4(
    vec4(0.0, 12.0, 3.0, 15.0),
    vec4(8.0, 4.0, 11.0, 7.0),
    vec4(2.0, 14.0, 1.0, 13.0),
    vec4(10.0, 6.0, 9.0, 5.0)
);

    //vec3 base_color = pow(vec3(0.0941, 0.0784, 0.10196), vec3(2.2));
    ////vec3 sky = vec3(0.3419, 0.2878, 0.542);
    //vec3 sky = base_color;
    ////vec3 horizon = vec3(0.3419, 0.2178, 0.452);
    //vec3 horizon = base_color;

void main()
{

    vec2 uv = tex_uv;
    FragColor = texture(bg_tex, uv);
return;
    vec2 pixel = tex_uv * vec2(320.0, 180.0);
    int x = int(pixel.x);
    int y = int(pixel.y);
    int dither_x = x & 3;
    int dither_y = y & 3;

    vec3 col = mix(vec3(1, 1, 1), vec3(0, 0, 0), (tex_uv.y * 0.5) - 0.5);

    float yoff = ((dither[dither_y][dither_x] * 10.0) - 5.0) / 180.0;
    float brightness = length(col / length(vec3(1, 1, 1)));
    uv.y = clamp(uv.y + yoff, 0, 1);//(brightness > dither[dither_y][dither_x]) ? clamp(uv.y + yoff, 0, 1) : uv.y;

    //FragColor = vec4(brightness, brightness, brightness, 1.0);
    //FragColor = vec4(col, 1.0);
}
)SRC";

RenderTarget make_render_to_texture_target(i32 w, i32 h)
{
    RenderTarget result;

    result.w = w;
    result.h = h;
    result.l = 1;
    TextureConfig tex_config;
    tex_config.width = w;
    tex_config.height = h;
    tex_config.src_format = GL_RGBA;
    tex_config.internal_format = GL_RGBA16F;
    tex_config.src_data_type = GL_FLOAT;
    tex_config.wrap_s = GL_CLAMP_TO_EDGE;
    tex_config.wrap_t = GL_CLAMP_TO_EDGE;
    result.target_texture = make_texture(tex_config);

    // create the internal framebuffer
    glGenFramebuffers(1, &result.target_framebuf);
    glBindFramebuffer(GL_FRAMEBUFFER, result.target_framebuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.target_texture.id, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return result;
}

RenderTarget make_render_to_array_texture_target(i32 w, i32 h, i32 layers, usize format)
{
    RenderTarget result;
    result.w = w;
    result.h = h;
    result.l = layers;
    result.target_texture = alloc_array_texture(w, h, layers, format);

    glGenFramebuffers(1, &result.target_framebuf);
    glBindFramebuffer(GL_FRAMEBUFFER, result.target_framebuf);
    // NOTE: wrong call?
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, result.target_texture.id, 0);
    return result;
}

#ifdef RIGEL_DEBUG

void render_debug_init(mem::Arena* gfx_arena)
{
    // lines buffers
    glGenBuffers(1, &debug_state.lines_vbo);
    glGenVertexArrays(1, &debug_state.lines_vao);

    glBindVertexArray(debug_state.lines_vao);
    glBindBuffer(GL_ARRAY_BUFFER, debug_state.lines_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    //lines shader
    Shader* dbg_line_shader = &game_shaders[DEBUG_LINE_SHADER];
    TextResource dbg_line_vss = load_text_resource("resource/shader/vs_dbg_line.glsl");
    TextResource dbg_line_fss = load_text_resource("resource/shader/fs_dbg_line.glsl");
    dbg_line_shader->load_from_src(dbg_line_vss.text, dbg_line_fss.text);
}

void render_debug_lines()
{
    usize n_lines;
    debug::DebugLine* lines = debug::get_lines_for_frame(&n_lines);

    glBindVertexArray(debug_state.lines_vao);
    glBindBuffer(GL_ARRAY_BUFFER, debug_state.lines_vbo);

    glBufferData(GL_ARRAY_BUFFER, n_lines * sizeof(debug::DebugLine), lines, GL_DYNAMIC_DRAW);

    m::Mat4 screen_transform =
        m::scale_by(m::Vec3 { 2.0f / 320.0f, 2.0f / 180.0f, 1.0f }) *
        m::translation_by(m::Vec3 {-1.0f, -1.0f, 0.0f});
    Shader shader = game_shaders[DEBUG_LINE_SHADER];
    glUseProgram(shader.id);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "screen_transform"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&screen_transform));

    glDrawArrays(GL_LINES, 0, 2 * n_lines);
    glBindVertexArray(0);
}

#endif

void initialize_renderer(mem::Arena* gfx_arena, f32 fb_width, f32 fb_height)
{
    render_state.gfx_arena = gfx_arena;

    // global UBO
    glGenBuffers(1, &render_state.global_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, render_state.global_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalUniforms), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, render_state.global_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    render_state.internal_target = make_render_to_texture_target(320, 180);

    render_state.shadowmap_target = make_render_to_array_texture_target(320, 180, 24, GL_RGBA);

    render_state.screen_target.w = fb_width;
    render_state.screen_target.h = fb_height;
    render_state.screen_target.target_framebuf = 0;

    Shader* screen_shader = &game_shaders[SCREEN_SHADER];
    // TODO: use proper resources here
    screen_shader->load_from_src(screen_vs_src, screen_fs_src);

    Shader* background_gradient_shader = &game_shaders[BACKGROUND_GRADIENT_SHADER];
    background_gradient_shader->load_from_src(screen_vs_src, fs_gradient);

    Shader* map_shader = &game_shaders[TILEMAP_DRAW_SHADER];
    TextResource map_shader_vs = load_text_resource("resource/shader/vs_map_batched.glsl");
    TextResource map_shader_fs = load_text_resource("resource/shader/fs_map_batched.glsl");
    map_shader->load_from_src(map_shader_vs.text, map_shader_fs.text);

    Shader* entity_shader = &game_shaders[ENTITY_DRAW_SHADER];
    TextResource entity_shader_vs = load_text_resource("resource/shader/vs_entity.glsl");
    TextResource entity_shader_fs = load_text_resource("resource/shader/fs_entity.glsl");
    entity_shader->load_from_src(entity_shader_vs.text, entity_shader_fs.text);

    Shader* shadow_shader = &game_shaders[TILE_OCCLUDER_SHADER];
    TextResource shadow_shader_vs = load_text_resource("resource/shader/vs_shadowmap.glsl");
    TextResource shadow_shader_fs = load_text_resource("resource/shader/fs_shadowmap.glsl");
    shadow_shader->load_from_src(shadow_shader_vs.text, shadow_shader_fs.text);

    render_state.screen.initialize();

    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    assets->ready_shaders = gfx_arena->alloc_simple<ShaderLookup>();
    assets->ready_textures = gfx_arena->alloc_simple<TextureLookup>();
    for (i32 i = 0; i < 64; i++) {
        assets->ready_textures->map[i].resource_id = RESOURCE_ID_NONE;
        assets->ready_textures->map[i].texture_idx = RESOURCE_ID_NONE;
    }
    assets->renderable_world_maps = gfx_arena->alloc_array<WorldChunkDrawData>(2);

    ImageResource bg_image = load_image_resource("resource/image/Clouds/Clouds 7/1.png");
    render_state.bg_image_id = bg_image.resource_id;

#ifdef RIGEL_DEBUG
    render_debug_init(gfx_arena);
#endif

}

m::Vec4 linear_to_srgb(m::Vec4 rgba)
{
    return m::Vec4{powf(rgba.r, 2.2), powf(rgba.g, 2.2), powf(rgba.b, 2.2), rgba.a};
}

void begin_render(Viewport& viewport, GameState* game_state, f32 fb_width, f32 fb_height)
{
    render_state.screen_target.w = fb_width;
    render_state.screen_target.h = fb_height;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    render_state.current_viewport.x = 0;
    render_state.current_viewport.y = 0;
    render_state.current_viewport.w = fb_width;
    render_state.current_viewport.h = fb_height;

    glViewport(0, 0, render_state.screen_target.w, render_state.screen_target.h);
    // NOTE: retrieved from tilesheet
    m::Vec4 clear_color = linear_to_srgb(m::Vec4{0.0941, 0.0784, 0.10196, 1});
    //m::Vec4 clear_color = linear_to_srgb(m::Vec4{0.3019, 0.6178, 0.902, 1});
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT);

    auto active_chunk = game_state->active_world_chunk;
    // TODO: obs should have lights be their own thing somewhere
    auto player = active_chunk->entities;
    auto player_collider = player->colliders->aabbs[0];
    m::Vec3 player_center = player->position + player_collider.extents;

    render_state.global_uniforms.screen_transform = viewport.get_screen_transform();
    i32 l;
    for (l = 0; l < active_chunk->next_free_light_idx; l++)
    {
        auto light = active_chunk->lights + l;
        auto uniform = render_state.global_uniforms.point_lights + l;
        // heck!
        float light_z = 5.0f;
        uniform->position = m::Vec4{light->position.x, light->position.y, light_z, 1};
        uniform->color = m::Vec4{light->color.x, light->color.y, light->color.z, 1};
    }
    UniformLight player_point_light;
    player_point_light.position = m::Vec4 {player_center.x, player_center.y, 10.0f, 1.0f};
    player_point_light.color = m::Vec4 {2.0f, 2.0f, 1.0f, 1.0f}; // fourth component could be strength?
    render_state.global_uniforms.point_lights[l] = player_point_light;
    render_state.global_uniforms.n_lights.x = active_chunk->next_free_light_idx + 1;

    glBindBuffer(GL_UNIFORM_BUFFER, render_state.global_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalUniforms), &render_state.global_uniforms);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void lighting_pass(mem::Arena* scratch_arena, TileMap* tile_map)
{
    // TODO
    //
    // I believe what I want is actually two different types of lights:
    // - classic point lights
    // - limited area lights that cast a relatively uniform value over a discrete
    //   circle
    usize n_lights = render_state.global_uniforms.n_lights.x;

    // TODO: This should take an optional clear color
    begin_render_to_target(render_state.shadowmap_target);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (usize light_idx = 0;
         light_idx < n_lights;
         light_idx++)
    {
        UniformLight* light = render_state.global_uniforms.point_lights + light_idx;
        m::Vec4 point_light4 = light->position;
        m::Vec3 point_light {point_light4.x, point_light4.y, point_light4.z};
        make_shadow_map_for_point_light(scratch_arena, tile_map, point_light, light_idx);

#ifdef RIGEL_DEBUG
        Rectangle point_light_rect;
        point_light_rect.x = light->position.x - 1;
        point_light_rect.y = light->position.y - 1;
        point_light_rect.w = 2;
        point_light_rect.h = 2;
        debug::push_rect_outline(point_light_rect, {light->color.r, light->color.g, light->color.b});
#endif

    }
    end_render_to_target();

}

void render_background()
{
    GpuQuad screen = render_state.screen;
    Shader screen_shader = game_shaders[BACKGROUND_GRADIENT_SHADER];

    auto bg_tex = get_renderable_texture(render_state.bg_image_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bg_tex->id);

    m::Mat4 world_transform =
        m::translation_by(m::Vec3 {-0.5, -0.5, 0.0}) *
        m::scale_by(m::Vec3 {2.0f, 2.0f, 0.0f });

    m::Mat4 screen_transform =
        m::mat4_I();
        //m::scale_by(
        //m::Vec3 {scale_factor * (2.0f / render_state.internal_target.w),
                //-scale_factor * (2.0f / render_state.internal_target.h), 0});

    glUseProgram(screen_shader.id);
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "screen_transform"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&screen_transform));
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "world_transform"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&world_transform));
    glUniform1i(glGetUniformLocation(screen_shader.id, "bg_tex"), 0);

    glBindVertexArray(screen.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

void render_background_layer(Viewport& viewport, WorldChunk* world_chunk)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_maps + world_chunk->level_index;
    assert(draw_data->renderable && "Tried to render an unready background tile map");

    glUseProgram(game_shaders[TILEMAP_DRAW_SHADER].id);
    glUniform1f(glGetUniformLocation(game_shaders[TILEMAP_DRAW_SHADER].id, "dimmer"), 0.9f);
    draw_data->bg_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void render_foreground_layer(Viewport& viewport, WorldChunk* world_chunk)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_maps + world_chunk->level_index;
    assert(draw_data->renderable && "Tried to render an unready foreground tile map");

    glUseProgram(game_shaders[TILEMAP_DRAW_SHADER].id);
    glUniform1f(glGetUniformLocation(game_shaders[TILEMAP_DRAW_SHADER].id, "dimmer"), 1.0f);
    draw_data->fg_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void render_decoration_layer(Viewport& viewport, WorldChunk* world_chunk)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(render_state.gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_maps + world_chunk->level_index;
    assert(draw_data->renderable && "Tried to render an unready decoration tile map");

    glUseProgram(game_shaders[TILEMAP_DRAW_SHADER].id);
    glUniform1f(glGetUniformLocation(game_shaders[TILEMAP_DRAW_SHADER].id, "dimmer"), 1.0f);
    draw_data->deco_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void make_shadow_map_for_point_light(mem::Arena* scratch_arena, TileMap* tile_map, m::Vec3 light_pos, i32 light_index)
{
    // we just need vec2 verts
    scratch_arena->reinit();

    usize n_tiles = tile_map->n_nonempty_tiles;
    usize elems_per_quad = 4 * 3; // 4 verts, 3 components each
    usize idx_elems_per_quad = 6;
    f32* verts = scratch_arena->alloc_array<f32>(n_tiles * elems_per_quad * 4); // times 2 since we output max 2 quads per tile
    u32* indices = scratch_arena->alloc_array<u32>(n_tiles * idx_elems_per_quad * 4);

    usize quad_idx = 0;

    m::Vec2 light_tilespace {light_pos.x, 180.0f - light_pos.y};

    for (usize y = 0; y < WORLD_HEIGHT_TILES; y++)
    {
        for (usize x = 0; x < WORLD_WIDTH_TILES; x++)
        {
            m::Vec2 dir_to_light =
                m::Vec2{(f32)(x * TILE_WIDTH_PIXELS), (f32)(y * TILE_WIDTH_PIXELS)} - light_tilespace;
            usize i = tile_to_index(x, y);

            if (tile_map->tiles[i] == TileType::EMPTY)
            {
                continue;
            }

            m::Vec2 tile_verts[] = {
                {(f32)(x * TILE_WIDTH_PIXELS),                       (f32)(y * TILE_WIDTH_PIXELS)},
                {(f32)((x * TILE_WIDTH_PIXELS) + TILE_WIDTH_PIXELS), (f32)(y * TILE_WIDTH_PIXELS)},
                {(f32)((x * TILE_WIDTH_PIXELS) + TILE_WIDTH_PIXELS), (f32)((y * TILE_WIDTH_PIXELS) + TILE_WIDTH_PIXELS)},
                {(f32)(x * TILE_WIDTH_PIXELS),                       (f32)((y * TILE_WIDTH_PIXELS) + TILE_WIDTH_PIXELS)}
            };

            for (usize edge_start = 0; edge_start < 4; edge_start++)
            {
                usize edge_end = edge_start + 1;
                if (edge_end >= 4)
                {
                    edge_end = 0;
                }

                m::Vec2 edge = tile_verts[edge_end] - tile_verts[edge_start];
                m::Vec2 norm {-edge.y, edge.x};
                if (m::dot(norm, dir_to_light) <= 0)
                {
                    continue;
                }

                m::Vec2 end_from_light = tile_verts[edge_end] - light_tilespace;
                m::Vec2 start_from_light = tile_verts[edge_start] - light_tilespace;

                usize quad_start = quad_idx * elems_per_quad;
                verts[quad_start] = tile_verts[edge_start].x;
                verts[quad_start + 1] = tile_verts[edge_start].y;
                verts[quad_start + 2] = 1.0;

                verts[quad_start + 3] = tile_verts[edge_end].x;
                verts[quad_start + 4] = tile_verts[edge_end].y;
                verts[quad_start + 5] = 1.0;

                verts[quad_start + 6] = end_from_light.x;
                verts[quad_start + 7] = end_from_light.y;
                verts[quad_start + 8] = 0.0;

                verts[quad_start + 9] = start_from_light.x;
                verts[quad_start + 10] = start_from_light.y;
                verts[quad_start + 11] = 0.0;

                usize idxs_start = quad_idx * idx_elems_per_quad;
                usize quad_elems_start = quad_idx * 4;
                indices[idxs_start] = quad_elems_start + 0;
                indices[idxs_start + 1] = quad_elems_start + 1;
                indices[idxs_start + 2] = quad_elems_start + 2;
                indices[idxs_start + 3] = quad_elems_start + 2;
                indices[idxs_start + 4] = quad_elems_start + 3;
                indices[idxs_start + 5] = quad_elems_start + 0;

                quad_idx += 1;
            }
        }
    }

    glBindVertexArray(0);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, quad_idx * elems_per_quad * sizeof(f32), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_idx * idx_elems_per_quad * sizeof(u32), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    auto shader = game_shaders[TILE_OCCLUDER_SHADER];
    glUseProgram(shader.id);

    m::Mat4 world_transform =
        m::translation_by({0.0f, (f32)(-WORLD_HEIGHT_TILES * TILE_WIDTH_PIXELS), 0.0f}) *
        m::scale_by({1.0f, -1.0f, 0.0f});

    glUniformMatrix4fv(glGetUniformLocation(shader.id, "world"), 1, GL_FALSE, reinterpret_cast<f32*>(&world_transform));

    glUniform1i(glGetUniformLocation(shader.id, "light_idx"), light_index);

    glDrawElements(GL_TRIANGLES, 6 * quad_idx, GL_UNSIGNED_INT, 0);

    scratch_arena->reinit();
}

void render_all_entities(Viewport& viewport, WorldChunk* world_chunk)
{
    auto screen = viewport.get_screen_transform();
    for (EntityId eid = 0; eid < world_chunk->next_free_entity_idx; eid++) {
        auto e = world_chunk->entities + eid;
        Texture* e_sprite = get_renderable_texture(e->sprite_id);
        ImageResource img_resource = get_image_resource(e->sprite_id);

        f32 vertical_scale = img_resource.height / img_resource.n_frames;
        // TODO: should have the concept of a sprite offset somewhere
        m::Mat4 world =
            m::scale_by({(f32)img_resource.width, vertical_scale, 0.0f}) *
            m::translation_by({e->position.x - 6.0f, e->position.y, 0.0f});

        auto shader = game_shaders[ENTITY_DRAW_SHADER];
        glUseProgram(shader.id);
        glUniformMatrix4fv(glGetUniformLocation(shader.id, "screen"),
                        1,
                        false,
                        reinterpret_cast<f32*>(&screen));
        glUniformMatrix4fv(glGetUniformLocation(shader.id, "world"),
                        1,
                        false,
                        reinterpret_cast<f32*>(&world));
        glUniform1i(glGetUniformLocation(shader.id, "anim_frame"), e->animation.current_frame);
        glUniform1i(glGetUniformLocation(shader.id, "sprite"), 0);
        // TODO: we need to update everything
        if (e->facing_dir.last_observed_sign == 0) {
            e->facing_dir.last_observed_sign = 1;
        }
        glUniform1i(glGetUniformLocation(shader.id, "facing_mult"), e->facing_dir.last_observed_sign);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, e_sprite->id);

        // TODO: screen tho? I mean, ideally we'd batch
        glBindVertexArray(render_state.screen.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }
}

void begin_render_to_target(RenderTarget target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.target_framebuf);

    render_state.current_viewport.x = 0;
    render_state.current_viewport.y = 0;
    render_state.current_viewport.w = target.w;
    render_state.current_viewport.h = target.h;

    glViewport(0, 0, target.w, target.h);
    m::Vec4 clear_color = linear_to_srgb(m::Vec4{0.0941, 0.0784, 0.10196, 1});
    //m::Vec4 clear_color = linear_to_srgb(m::Vec4{0.3019, 0.6178, 0.802, 1});
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}


void begin_render_to_internal_target()
{
    begin_render_to_target(render_state.internal_target);
}

void end_render_to_target()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    render_state.current_viewport.x = 0;
    render_state.current_viewport.y = 0;
    render_state.current_viewport.w = render_state.screen_target.w;
    render_state.current_viewport.h = render_state.screen_target.h;

    glViewport(render_state.current_viewport.x,
               render_state.current_viewport.y,
               render_state.current_viewport.w,
               render_state.current_viewport.h);
}

void end_render()
{
    // put the quad on the screen
    f32 fb_width = render_state.internal_target.w;
    f32 fb_height = render_state.internal_target.h;
    GpuQuad screen = render_state.screen;
    Shader screen_shader = game_shaders[SCREEN_SHADER];
    f32 scale_factor = render_state.screen_target.w / render_state.internal_target.w;

    m::Mat4 world_transform =
        m::translation_by(m::Vec3 {-0.5, -0.5, 0.0}) *
        m::scale_by(m::Vec3 {fb_width, fb_height, 0.0f });

    m::Mat4 screen_transform =
        m::scale_by(
        m::Vec3 {scale_factor * (2.0f / render_state.screen_target.w),
                -scale_factor * (2.0f / render_state.screen_target.h), 0});

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_state.internal_target.target_texture.id);

    glUseProgram(screen_shader.id);
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "screen_transform"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&screen_transform));
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "world_transform"),
                       1,
                       false,
                       reinterpret_cast<f32*>(&world_transform));
    glUniform1i(glGetUniformLocation(screen_shader.id, "game"),
                       0);

    glBindVertexArray(screen.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

#ifdef RIGEL_DEBUG
    render_debug_lines();
#endif
}

RenderTarget internal_target()
{
    return render_state.internal_target;
}

void draw_rectangle(Rectangle rect, f32 r, f32 g, f32 b)
{
    // real dull way to just draw a rectangle
    glEnable(GL_SCISSOR_TEST);
    glScissor(rect.x, rect.y, rect.w, rect.h);
    glClearColor(r, g, b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

} // namespace render
} // namespace rigel
