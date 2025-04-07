#version 400 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

uniform mat4 screen;
uniform mat4 world;
uniform vec4 color;

out vec2 tex_coord;
out vec4 f_color;

void main()
{
    gl_Position = screen * world * vec4(pos, 1.0);
    tex_coord = uv;
    f_color = color;
}
