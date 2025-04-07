#version 400 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

uniform mat4 world_transform;
uniform mat4 screen_transform;
// TODO: viewport uniform

out vec2 tex_coord;

void main()
{
    gl_Position = screen_transform * world_transform * vec4(pos, 1.0);
    tex_coord = uv;
}
