#version 400 core
#extension GL_ARB_shader_viewport_layer_array : enable

layout (location = 0) in vec3 pos;

uniform mat4 world;
uniform int light_idx;

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

void main()
{
    gl_Position = screen * world * vec4(pos.x, pos.y, 0.0, pos.z);
    gl_Layer = light_idx;
}
