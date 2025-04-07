#version 400 core

in vec2 tex_coord;

out vec4 FragColor;

uniform uint width;
uniform uint height;
uniform sampler2DArray atlas;
uniform usamplerBuffer tiles;

void main()
{
    vec2 map_coord = vec2(tex_coord.x * width, tex_coord.y * height);
    vec2 grid_coord = floor(map_coord);
    vec2 tile_uv = fract(map_coord);

    int tileno = int((grid_coord.y * width) + grid_coord.x);
    float sprite = texelFetch(tiles, tileno).r;

    if (sprite == 0) {
        FragColor = vec4(0.078, 0.047, 0.11, 1.0);
    } else {
        FragColor = texture(atlas, vec3(tile_uv, sprite - 1));
    }
}
