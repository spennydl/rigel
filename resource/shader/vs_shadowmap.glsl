#version 400 core
#extension GL_ARB_shader_viewport_layer_array : enable

layout (location = 0) in vec3 pos;

uniform mat4 world;
uniform int light_idx;

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    vec3 point_lights[24];
};

void main()
{
    gl_Position = screen * world * vec4(pos.x, pos.y, 0.0, pos.z);
    gl_Layer = light_idx;
}
