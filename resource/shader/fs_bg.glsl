#version 400 core

in vec2 tex_coord;

uniform sampler2D graphic;

out vec4 FragColor;

void main()
{
    FragColor = texture(graphic, tex_coord);
    //FragColor = vec4(tex_coord, 0.0, 1.0);

}
