#version 400 core

in vec2 tex_coord;
in vec4 f_color;

out vec4 FragColor;

void main()
{
    FragColor = f_color;
}
