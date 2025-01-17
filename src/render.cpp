#include "render.h"
#include "world.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO: Prolly don't need this, hey?
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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
Shader::load_from_src(const std::string& vs_src, const std::string& fs_src)
{
    GLuint vs, fs;
    vs = glCreateShader(GL_VERTEX_SHADER);
    auto vs_csrc = vs_src.c_str();
    glShaderSource(vs, 1, &vs_csrc, nullptr);
    glCompileShader(vs);
    if (!check_shader_status(vs)) {
        std::cerr << "vertex failed" << std::endl;
        return;
    }

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    auto fs_csrc = fs_src.c_str();
    glShaderSource(fs, 1, &fs_csrc, nullptr);
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

Shader::Shader(const std::string& vs_src, const std::string& fs_src)
{
    load_from_src(vs_src, fs_src);
}

Texture::Texture(const std::string& path)
{
    int w, h, channels;

    auto image_data = stbi_load(path.c_str(), &w, &h, &channels, 3);
    if (image_data == nullptr) {
        std::cerr << "Couldn't load atlas texture" << std::endl;
        return;
    }

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
                 image_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image_data);
}

#if 0
SpriteAtlas::SpriteAtlas(const std::string& path, int width, int height)
{
    int w, h, channels;

    auto image_data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    channels = 4;
    if (image_data == nullptr) {
        std::cerr << "Couldn't load atlas texture" << std::endl;
        return;
    }

    int tiles_in_row = w / width;
    int tiles_in_col = h / height;
    this->tiles = tiles_in_row * tiles_in_col;

    glGenTextures(1, &this->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 1);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, this->tiles, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    int zoffset = 0;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
    for (int y = 0; y < tiles_in_col; y++) {
        for (int x = 0; x < tiles_in_row; x++) {
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, x * width);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, y * height);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zoffset++, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    delete[] image_data;
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idx), quad_idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void MapDrawLayer::buffer_map(const TileMap& tiles)
{
    if (this->map_buf != 0) {
        std::cout << "rebuffering map!" << std::endl;
        return;
    }

    int mapsize = tiles.width * tiles.height;

    glGenBuffers(1, &this->map_buf);
    glBindBuffer(GL_TEXTURE_BUFFER, this->map_buf);
    glBufferData(GL_TEXTURE_BUFFER, mapsize * sizeof(uint16_t), tiles.sprites, GL_STATIC_DRAW);

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

    glUniform1ui(glGetUniformLocation(map.shader_prog.id, "width"), 30);
    glUniform1ui(glGetUniformLocation(map.shader_prog.id, "height"), 28);
    glUniform1i(glGetUniformLocation(map.shader_prog.id, "atlas"), 0);
    glUniform1i(glGetUniformLocation(map.shader_prog.id, "tiles"), 1);

    glBindVertexArray(map.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

#endif

Quad::Quad(rigel::phys::Rectangle rect, Shader shader)
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idx), quad_idx, GL_STATIC_DRAW);

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
      glm::vec3(this->scale * (2.0 / 1280.0), this->scale * (2.0 / 720.0), 0));
    screen_transform = glm::translate(screen_transform, this->position * -1.0f);

    return screen_transform;
}

void
Viewport::zoom(float factor)
{
    this->scale = factor;
}

const char* vector_vs_src = "#version 400 core\n"
                            "layout (location = 0) in vec3 vert;\n"
                            "layout (location = 1) in vec3 color;\n"
                            "out vec3 f_color;\n"
                            "uniform mat4 screen_transform;\n"
                            "void main() {\n"
                            "gl_Position = screen_transform * vec4(vert, 1.0);"
                            "f_color = color;\n"
                            "}";

const char* vector_fs_src = "#version 400 core\n"
                            "in vec3 f_color;\n"
                            "out vec4 FragColor;\n"
                            "void main(){"
                            "FragColor = vec4(f_color, 1.0);\n"
                            "}";

VectorRenderer::VectorRenderer()
  : lines()
  , needs_rebuffer(true)
  , line_shader()
{
    line_shader.load_from_src(vector_vs_src, vector_fs_src);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int
VectorRenderer::add_line(RenderLine line)
{
    lines.push_back(line);
    needs_rebuffer = true;
    return lines.size() - 1;
}

void
VectorRenderer::update_line(int idx, RenderLine new_line)
{
    lines[idx] = new_line;
    needs_rebuffer = true;
}

void
VectorRenderer::render(Viewport& viewport)
{
    if (needs_rebuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     lines.size() * sizeof(RenderLine),
                     lines.data(),
                     GL_STREAM_DRAW);
        needs_rebuffer = false;
    }
    glBindVertexArray(vao);
    auto screen_transform = viewport.get_screen_transform();
    glUseProgram(line_shader.id);
    glUniformMatrix4fv(glGetUniformLocation(line_shader.id, "screen_transform"),
                       1,
                       false,
                       glm::value_ptr(screen_transform));
    glDrawArrays(GL_LINES, 0, lines.size() * 2);
}

GpuQuad::GpuQuad()
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idx), quad_idx, GL_STATIC_DRAW);

    glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

SpriteAtlas::SpriteAtlas()
{
    for (int i = 0; i < MAX_ATLAS_SPRITES; i++) {
        loaded_sprites[i].resource_id = SPRITE_RESOURCE_ID_NONE;
    }
}

Sprite*
SpriteAtlas::lookup(SpriteResourceId id)
{
    auto hash = id & (MAX_ATLAS_SPRITES - 1);
    if (loaded_sprites[hash].resource_id == id) {
        return sprites + loaded_sprites[hash].idx;
    }
    return nullptr;
}

SpriteResourceId
SpriteAtlas::load_sprite(SpriteResourceId resource_id,
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

void
SpriteRenderStep::render_entities_in_world(Viewport* viewport,
                                           WorldChunk* world_chunk)
{
    // TODO: need to sort entities
    for (u32 i = 0; i < world_chunk->next_free_entity_idx; i++) {
        auto entity = &world_chunk->entities[i];
        auto sprite = atlas->lookup(entity->sprite_id);
        assert(sprite && "Trying to render unloaded sprite!");

        auto screen_transform = viewport->get_screen_transform();
        phys::Rectangle quad_rect = { .x = entity->position.x,
                                      .y = entity->position.y,
                                      .w = sprite->width,
                                      .h = sprite->height };
        auto world_transform = quad_rect.get_world_transform();

        glUseProgram(sprite_shader.id);
        glUniform1i(glGetUniformLocation(sprite_shader.id, "sprite_tex"), 0);
        glUniformMatrix4fv(
          glGetUniformLocation(sprite_shader.id, "screen_transform"),
          1,
          false,
          glm::value_ptr(screen_transform));
        glUniformMatrix4fv(
          glGetUniformLocation(sprite_shader.id, "world_transform"),
          1,
          false,
          glm::value_ptr(world_transform));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite->tex_id);
        glBindVertexArray(quad.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // There are some other optimizations we may be able to make here.
        // For one, there will likely be more than one entity on screen using
        // the same sprite but not enough to jump to instancing (that will be
        // later). Grouping entities by sprite could be a good way to cut down
        // on the number of texture binds we have to do. We likely won't have a
        // lot of entities so this might not actually be worth it but it's a
        // thought.
    }
}

} // rigel
} // render
