#version 400 core

uniform sampler2DArray sprite;
uniform int anim_frame;
uniform int facing_mult;

in vec2 tex_coord;

out vec4 FragColor;

void main()
{
    vec2 uv = vec2(tex_coord.x * facing_mult, tex_coord.y);
    FragColor = texture(sprite, vec3(uv, anim_frame));
}
