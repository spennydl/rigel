#version 400 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

uniform mat4 screen_transform;

out vec3 f_color;

void main()
{
    gl_Position = screen_transform * vec4(pos, 1.0);
    f_color = color;
}
