#include "render.h"
#include "world.h"
#include "resource.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>

// TODO: remove
#include <string>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace rigel {
namespace render {

static Shader game_shaders[N_GAME_SHADERS];

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

    tile_atlas = make_texture(atlas_image);

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
    auto screen_transform = viewport.get_screen_transform();

    glm::mat4 world_transform(1.0);
    world_transform = glm::scale(world_transform, glm::vec3(8.0, -8.0, 0));
    world_transform = glm::translate(world_transform, glm::vec3(0.0, -WORLD_HEIGHT_TILES, 0.0));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tile_atlas.id);

    glUseProgram(shader.id);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "world"), 1, GL_FALSE, glm::value_ptr(world_transform));
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "screen"), 1, GL_FALSE, glm::value_ptr(screen_transform));
    glUniform1i(glGetUniformLocation(shader.id, "atlas"), 0);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6 * n_tiles, GL_UNSIGNED_INT, 0);
}

Texture alloc_texture(int w, int h)
{
    Texture tex;

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 w,
                 h,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

Texture make_texture(int w, int h, ubyte* data)
{
    Texture tex;

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 w,
                 h,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);
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

Texture make_texture(ImageResource image)
{
    return make_texture(image.width, image.height, image.data);
}

Texture make_array_texture_from_vstrip(ImageResource image, usize n_images)
{
    Texture tex;
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.id);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);

    usize slice_height = image.height / n_images;

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 GL_RGBA8,
                 image.width,
                 slice_height,
                 n_images,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 image.data);

    // Do I need this?
    // glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

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
                       glm::value_ptr(screen_transform));
    glUniformMatrix4fv(glGetUniformLocation(quad.shader.id, "world"),
                       1,
                       false,
                       glm::value_ptr(quad_world));
    glUniform4f(glGetUniformLocation(quad.shader.id, "color"),
                (float)r / 255.0,
                (float)g / 255.0,
                (float)b / 255.0,
                1.0);

    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Tri::Tri(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, Shader shader)
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
                       glm::value_ptr(screen_transform));
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
Viewport::translate(glm::vec3 by)
{
    this->position += by;
}

glm::vec3
Viewport::get_position() const noexcept
{
    return this->position;
}

glm::mat4
Viewport::get_screen_transform() const
{
    glm::mat4 screen_transform(1.0f);
    screen_transform = glm::translate(screen_transform, glm::vec3(-1, -1, 0));
    screen_transform = glm::scale(
      screen_transform,
      glm::vec3(this->scale * (2.0 / this->width),
                this->scale * (2.0 / this->height), 0));
    screen_transform = glm::translate(screen_transform, this->position * -1.0f);

    return screen_transform;
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

Shader* get_renderable_shader(mem::Arena* gfx_arena, TextResource vs_src, TextResource fs_src)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    ShaderLookup* ready_shaders = assets->ready_shaders;

    usize next_idx = ready_shaders->next_free_shader_idx;
    assert(next_idx < 64 && "Too many shaders at once");
    Shader* result = ready_shaders->shaders + next_idx;
    result->load_from_src(vs_src.text, fs_src.text);

    return result;
}

// TODO: can have a global RenderableAssets pointer since
// gfx arena will never move.
Texture* get_renderable_texture(mem::Arena* gfx_arena, ResourceId sprite_id)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    TextureLookup* ready_textures = assets->ready_textures;

    auto map_idx = sprite_id % 64;
    auto map_entry = ready_textures->map + map_idx;
    if (map_entry->resource_id == sprite_id) {
        return ready_textures->textures + map_entry->texture_idx;
    }

    if (map_entry->resource_id < 0) {
        usize next_idx = ready_textures->next_free_texture_idx;
        assert(next_idx < 64 && "Too many textures at once");

        Texture* result = ready_textures->textures + next_idx;

        ImageResource image_resource = get_image_resource(sprite_id);
        if (image_resource.n_frames == 1) {
            *result = make_texture(image_resource);
        } else {
            *result = make_array_texture_from_vstrip(image_resource, image_resource.n_frames);
        }

        map_entry->resource_id = map_idx;
        map_entry->texture_idx = next_idx;

        ready_textures->next_free_texture_idx++;

        return result;
    }

    assert(false && "Implement open addressing already you nerd");
}

void make_world_chunk_renderable(mem::GameMem* memory, WorldChunk* world_chunk, ImageResource tile_set)
{
    auto gfx_arena = &memory->gfx_arena;
    auto scratch_arena = &memory->scratch_arena;

    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_chunk;

    TileMap* map = world_chunk->active_map;

    draw_data->fg_renderer = BatchedTileRenderer(*scratch_arena, map, tile_set);
    draw_data->bg_renderer = BatchedTileRenderer(*scratch_arena, map->background, tile_set);
    draw_data->deco_renderer = BatchedTileRenderer(*scratch_arena, map->decoration, tile_set);
}

// TODO: don't think I need this fn
void make_entity_renderable(mem::Arena* gfx_arena, entity::Entity* e)
{
    Texture* entity_sprite = get_renderable_texture(gfx_arena, e->sprite_id);
    e->sprite_id = entity_sprite->ready_idx;
}

static RenderState render_state;

const char* screen_vs_src = "#version 400 core\n"
                            "layout (location = 0) in vec3 vert;\n"
                            "layout (location = 1) in vec2 uv;\n"
                            "out vec2 tex_uv;\n"
                            "uniform mat4 screen_transform;\n"
                            "uniform mat4 world_transform;\n"
                            "void main() {\n"
                            "gl_Position = screen_transform * world_transform * vec4(vert, 1.0);\n"
                            "tex_uv = uv;\n"
                            "}";

const char* screen_fs_src = "#version 400 core\n"
                            "in vec2 tex_uv;\n"
                            "uniform sampler2D game;\n"
                            "out vec4 FragColor;\n"
                            "void main(){\n"
                            "FragColor = texture(game, tex_uv);\n"
                            "}";

void initialize_renderer(mem::Arena* gfx_arena, f32 fb_width, f32 fb_height)
{
    // global UBO
    glGenBuffers(1, &render_state.global_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, render_state.global_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalUniforms), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    render_state.internal_target.w = 320;
    render_state.internal_target.h = 180;
    render_state.texture = alloc_texture(render_state.internal_target.w, render_state.internal_target.h);

    Shader* screen_shader = &game_shaders[SCREEN_SHADER];
    // TODO: use proper resources here
    screen_shader->load_from_src(screen_vs_src, screen_fs_src);

    Shader* map_shader = &game_shaders[TILEMAP_DRAW_SHADER];
    TextResource map_shader_vs = load_text_resource("resource/shader/vs_map_batched.glsl");
    TextResource map_shader_fs = load_text_resource("resource/shader/fs_map_batched.glsl");
    map_shader->load_from_src(map_shader_vs.text, map_shader_fs.text);

    Shader* entity_shader = &game_shaders[ENTITY_DRAW_SHADER];
    TextResource entity_shader_vs = load_text_resource("resource/shader/vs_entity.glsl");
    TextResource entity_shader_fs = load_text_resource("resource/shader/fs_entity.glsl");
    entity_shader->load_from_src(entity_shader_vs.text, entity_shader_fs.text);

    render_state.screen.initialize();

    // create the internal framebuffer
    glGenFramebuffers(1, &render_state.internal_target.target_framebuf);
    glBindFramebuffer(GL_FRAMEBUFFER, render_state.internal_target.target_framebuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_state.texture.id, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    render_state.fb_width = fb_width;
    render_state.fb_height = fb_height;

    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    assets->ready_shaders = gfx_arena->alloc_simple<ShaderLookup>();
    assets->ready_textures = gfx_arena->alloc_simple<TextureLookup>();
    for (i32 i = 0; i < 64; i++) {
        assets->ready_textures->map[i].resource_id = RESOURCE_ID_NONE;
        assets->ready_textures->map[i].texture_idx = RESOURCE_ID_NONE;
    }
    assets->renderable_world_chunk = gfx_arena->alloc_simple<WorldChunkDrawData>();
}

void begin_render(f32 fb_width, f32 fb_height)
{
    render_state.fb_width = fb_width;
    render_state.fb_height = fb_height;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    render_state.current_viewport.x = 0;
    render_state.current_viewport.y = 0;
    render_state.current_viewport.w = fb_width;
    render_state.current_viewport.h = fb_height;

    glViewport(0, 0, render_state.fb_width, render_state.fb_height);
    glClearColor(0.0941, 0.0784, 0.10196, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: send down global uniforms here
}

void render_background_layer(mem::Arena* gfx_arena, Viewport& viewport)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_chunk;

    draw_data->bg_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void render_foreground_layer(mem::Arena* gfx_arena, Viewport& viewport)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_chunk;

    draw_data->fg_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void render_decoration_layer(mem::Arena* gfx_arena, Viewport& viewport)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    WorldChunkDrawData* draw_data = assets->renderable_world_chunk;

    draw_data->deco_renderer.render(viewport, game_shaders[TILEMAP_DRAW_SHADER]);
}

void render_all_entities(mem::Arena* gfx_arena, Viewport& viewport, WorldChunk* world_chunk, usize temp_anim_frame)
{
    RenderableAssets* assets = reinterpret_cast<RenderableAssets*>(gfx_arena->mem_begin);
    TextureLookup* texture_lookup = assets->ready_textures;

    auto screen = viewport.get_screen_transform();
    for (EntityId eid = 0; eid < world_chunk->next_free_entity_idx; eid++) {
        auto e = world_chunk->entities + eid;
        Texture* e_sprite = get_renderable_texture(gfx_arena, e->sprite_id);
        ImageResource img_resource = get_image_resource(e->sprite_id);

        f32 vertical_scale = img_resource.height / img_resource.n_frames;
        glm::mat4 world(1.0f);
        // TODO: should have the concept of a sprite offset somewhere
        world = glm::translate(world, glm::vec3(e->position.x - 6, e->position.y, 0));
        world = glm::scale(world, glm::vec3(img_resource.width, vertical_scale, 0.0f));

        auto shader = game_shaders[ENTITY_DRAW_SHADER];
        glUseProgram(shader.id);
        glUniformMatrix4fv(glGetUniformLocation(shader.id, "screen"),
                        1,
                        false,
                        glm::value_ptr(screen));
        glUniformMatrix4fv(glGetUniformLocation(shader.id, "world"),
                        1,
                        false,
                        glm::value_ptr(world));
        glUniform1i(glGetUniformLocation(shader.id, "anim_frame"), temp_anim_frame);
        glUniform1i(glGetUniformLocation(shader.id, "sprite"), 0);
        usize facing_mult = e->velocity.x > 0 ? 1 : -1;
        glUniform1i(glGetUniformLocation(shader.id, "facing_mult"), facing_mult);

        glBindTexture(GL_TEXTURE_2D_ARRAY, e_sprite->id);

        // TODO: screen tho? I mean, ideally we'd batch
        glBindVertexArray(render_state.screen.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
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
    glClearColor(0.0941, 0.0784, 0.10196, 1);
    //glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void end_render_to_target()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    render_state.current_viewport.x = 0;
    render_state.current_viewport.y = 0;
    render_state.current_viewport.w = render_state.fb_width;
    render_state.current_viewport.h = render_state.fb_height;
    glViewport(0, 0, render_state.fb_width, render_state.fb_height);
}

void end_render()
{
    // put the quad on the screen
    f32 fb_width = render_state.internal_target.w;
    f32 fb_height = render_state.internal_target.h;
    GpuQuad screen = render_state.screen;
    Shader screen_shader = game_shaders[SCREEN_SHADER];
    f32 scale_factor = 4.0;

    glm::mat4 world_transform(1.0f);
    world_transform = glm::scale(world_transform, glm::vec3(fb_width, fb_height, 0.0f));
    world_transform = glm::translate(world_transform, glm::vec3(-0.5, -0.5, 0));

    glm::mat4 screen_transform(1.0f);
    //screen_transform = glm::translate(screen_transform, glm::vec3(-0.5, -0.5, 0));
    screen_transform = glm::scale(
      screen_transform,
      glm::vec3(scale_factor * (2.0 / render_state.fb_width),
                -scale_factor * (2.0 / render_state.fb_height), 0));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_state.texture.id);

    glUseProgram(screen_shader.id);
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "screen_transform"),
                       1,
                       false,
                       glm::value_ptr(screen_transform));
    glUniformMatrix4fv(glGetUniformLocation(screen_shader.id, "world_transform"),
                       1,
                       false,
                       glm::value_ptr(world_transform));
    glUniform1i(glGetUniformLocation(screen_shader.id, "game"),
                       0);

    glBindVertexArray(screen.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // TODO: sdl stuff

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
