#version 400 core

out vec4 FragColor;

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    vec3 point_lights[24];
};

void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
