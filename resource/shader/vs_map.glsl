#version 400 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

out vec2 tex_coord;
out vec2 frag_pos;

void main()
{
    tex_coord = uv;
    gl_Position = vec4(pos.x * (480.0 / 1280.0), pos.y * (448.0 / 720.0), 0, 1);
}
