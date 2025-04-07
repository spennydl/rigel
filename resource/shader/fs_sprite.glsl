#version 400 core

in vec2 tex_coord;

out vec4 FragColor;

uniform sampler2D sprite_tex;

void main()
{
    FragColor = texture(sprite_tex, vec2(tex_coord.x, tex_coord.y));
}
