#version 400 core
layout (location = 0) in vec3 pos;

uniform mat4 screen;
uniform vec4 color;

out vec4 f_color;

void main()
{
    gl_Position = screen * vec4(pos, 1.0);
    f_color = color;
}
