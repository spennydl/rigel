#version 400 core
layout (location = 0) in vec3 pos;

uniform mat4 world;

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    vec3 point_lights[24];
};

void main()
{
    gl_Position = screen * world * vec4(pos.x, pos.y, 0.0, pos.z);
}
