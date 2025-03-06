#include "render.h"
#include "world.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>

// TODO: remove
#include <string>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace rigel {
namespace render {

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

// TODO: change path to just accept data
SpriteAtlas::SpriteAtlas(ImageResource image)
{
    int tiles_in_row = image.width / 8;
    int tiles_in_col = image.height / 8;
    this->tiles = tiles_in_row * tiles_in_col;

    glGenTextures(1, &this->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 8, 8, this->tiles, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    int zoffset = 0;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image.width);
    for (int y = 0; y < tiles_in_col; y++) {
        for (int x = 0; x < tiles_in_row; x++) {
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, x * 8);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, y * 8);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zoffset++, 8, 8, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

unsigned char* unpack_sprite_atlas(const std::string& path, int width, int height, int* out_levels)
{
    int w, h, channels;

    auto image_data = stbi_load(path.c_str(), &w, &h, &channels, 3);
    channels = 3;
    if (image_data == nullptr) {
        std::cerr << "Couldn't load atlas texture" << std::endl;
        return nullptr;
    }

    unsigned char* result = new unsigned char[w * h * channels];

    int sprite_width_bytes = width * channels;
    int w_bytes = w * channels;

    int n_rows = h / height;
    int n_cols = w / width;

    for (int tile_row = 0; tile_row < n_rows; tile_row++) {
        int common_offset = tile_row * (w_bytes * height);
        for (int pixel_row = 0; pixel_row < height; pixel_row++) {
            for (int x = 0; x < n_cols; x++) {
                int sprite_stride = x * sprite_width_bytes;
                int row_stride = pixel_row * w_bytes;
                int orig_offset = common_offset + row_stride + sprite_stride;

                int dest_sprite_stride = x * sprite_width_bytes * height;
                int dest_row_stride = pixel_row * sprite_width_bytes;

                std::copy(image_data + orig_offset,
                          image_data + orig_offset + sprite_width_bytes,
                          result + common_offset + dest_sprite_stride + dest_row_stride);
            }
        }
    }

    stbi_image_free(image_data);

    *out_levels = (w / width) * (h / height);
    //stbi_write_png("./test.png", width, height * *out_levels, 3, result, width * channels);

    return result;
}

MapDrawLayer::MapDrawLayer(SpriteAtlas atlas, Shader prog)
    : vao(0), ebo(0), map_buf(0), map_buf_tex(0), atlas(atlas), shader_prog(prog)
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_IDXS), QUAD_IDXS, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void MapDrawLayer::buffer_map(const TileMap* tiles)
{
    if (this->map_buf != 0) {
        std::cout << "rebuffering map!" << std::endl;
        return;
    }

    int mapsize = WORLD_SIZE_TILES;

    glGenBuffers(1, &this->map_buf);
    glBindBuffer(GL_TEXTURE_BUFFER, this->map_buf);
    glBufferData(GL_TEXTURE_BUFFER, mapsize * sizeof(u16), tiles->tile_sprites, GL_STATIC_DRAW);

    glGenTextures(1, &this->map_buf_tex);
    glBindTexture(GL_TEXTURE_BUFFER, this->map_buf_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, this->map_buf);
}

void render_map(const MapDrawLayer& map)
{
    glUseProgram(map.shader_prog.id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, map.atlas.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, map.map_buf_tex);

    glUniform1ui(glGetUniformLocation(map.shader_prog.id, "width"), 40);
    glUniform1ui(glGetUniformLocation(map.shader_prog.id, "height"), 23);
    glUniform1i(glGetUniformLocation(map.shader_prog.id, "atlas"), 0);
    glUniform1i(glGetUniformLocation(map.shader_prog.id, "tiles"), 1);

    glBindVertexArray(map.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

Texture::Texture(int w, int h)
{
    glGenTextures(1, &this->id);
    glBindTexture(GL_TEXTURE_2D, this->id);

    glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
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
}

Texture::Texture(int w, int h, ubyte* data)
{
    glGenTextures(1, &this->id);
    std::cout << "Gen tex " << this->id << ", wxh: " << w << " " << h << std::endl;
    glBindTexture(GL_TEXTURE_2D, this->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);
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

SpriteLibrary::SpriteLibrary()
{
    for (int i = 0; i < MAX_ATLAS_SPRITES; i++) {
        loaded_sprites[i].resource_id = SPRITE_RESOURCE_ID_NONE;
    }
}

Sprite*
SpriteLibrary::lookup(SpriteResourceId id)
{
    auto hash = id & (MAX_ATLAS_SPRITES - 1);
    if (loaded_sprites[hash].resource_id == id) {
        return sprites + loaded_sprites[hash].idx;
    }
    return nullptr;
}

SpriteResourceId
SpriteLibrary::load_sprite(SpriteResourceId resource_id,
                         usize width,
                         usize height,
                         ubyte* data)
{
    assert(next_free_idx + 1 < MAX_ATLAS_SPRITES &&
           "Trying to load too many sprites");

    auto sprite = lookup(resource_id);
    if (sprite != nullptr) {
        return sprite->id;
    }

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    i32 sprite_idx = next_free_idx;
    next_free_idx++;
    // TODO: use the sprite's texture id
    auto hash = resource_id & (MAX_ATLAS_SPRITES - 1);
    loaded_sprites[hash].idx = sprite_idx;
    loaded_sprites[hash].resource_id = resource_id;

    sprites[sprite_idx].id = resource_id;
    sprites[sprite_idx].tex_id = tex_id;
    sprites[sprite_idx].width = width;
    sprites[sprite_idx].height = height;

    return resource_id;
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

void initialize_renderer(f32 fb_width, f32 fb_height)
{
    // global UBO
    glGenBuffers(1, &render_state.global_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, render_state.global_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalUniforms), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    render_state.internal_target.w = 320;
    render_state.internal_target.h = 180;
    //Texture fb_texture("astro2.png");
    Texture fb_texture(render_state.internal_target.w, render_state.internal_target.h);
    render_state.texture = fb_texture;

    render_state.screen_shader.load_from_src(screen_vs_src, screen_fs_src);
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
}

void begin_render(f32 fb_width, f32 fb_height)
{
    render_state.fb_width = fb_width;
    render_state.fb_height = fb_height;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, render_state.fb_width, render_state.fb_height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: send down global uniforms here
}

void begin_render_to_target(RenderTarget target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.target_framebuf);
    glViewport(0, 0, target.w, target.h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void end_render_to_target()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, render_state.fb_width, render_state.fb_height);
}

void end_render()
{
    // put the quad on the screen
    f32 fb_width = render_state.internal_target.w;
    f32 fb_height = render_state.internal_target.h;
    GpuQuad screen = render_state.screen;
    Shader screen_shader = render_state.screen_shader;
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

}

void render_stage_background_layer(Viewport& viewport, Texture& texture, Shader& shader)
{
    auto quad = render_state.screen;

    glm::mat4 world_transform(1.0f);
    world_transform = glm::scale(world_transform, glm::vec3(RENDER_INTERNAL_WIDTH, RENDER_INTERNAL_HEIGHT, 0.0f));
    world_transform = glm::translate(world_transform, glm::vec3(-0.5, -0.5, 0));

    glm::mat4 screen_transform(1.0f);
    screen_transform = glm::scale(
      screen_transform,
      glm::vec3((2.0 / RENDER_INTERNAL_WIDTH),
                (2.0 / RENDER_INTERNAL_HEIGHT), 0));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    glUseProgram(shader.id);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "world_transform"),
                       1,
                       false,
                       glm::value_ptr(world_transform));

    glUniformMatrix4fv(glGetUniformLocation(shader.id, "screen_transform"),
                       1,
                       false,
                       glm::value_ptr(screen_transform));
    glUniform1i(glGetUniformLocation(shader.id, "graphic"), 0);

    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

RenderTarget internal_target()
{
    return render_state.internal_target;
}


} // namespace render
} // namespace rigel
