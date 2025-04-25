#version 400 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

uniform mat4 world;

struct Light
{
    vec4 position;
    vec4 color;
    vec4 data;
};

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    Light point_lights[24];
    Light circle_lights[24];
};

out vec2 tex_coord;
out vec3 frag_world_pos;

void main()
{
    gl_Position = screen * world * vec4(pos, 1.0);
    frag_world_pos = (world * vec4(pos, 1.0)).xyz;
    tex_coord = uv;
}
