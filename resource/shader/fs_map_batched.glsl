#version 400 core

in vec2 tex_coord;
in vec3 frag_world_pos;

out vec4 FragColor;

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    vec3 point_lights[24];
};

uniform sampler2D atlas;

void main()
{

    float color_mult = 0;
    vec3 frag = frag_world_pos;

    for (int light_index = 0; light_index < 2; light_index++) {
        vec3 light = point_lights[light_index];
        float distsq_to_light = length(light - frag);//abs(dot(light - frag, light - frag));
        float attenuation = 1.0 / (1.0 + (0.022*distsq_to_light + (0.0019 * distsq_to_light * distsq_to_light)));
        color_mult += attenuation;
    }

    //FragColor = vec4(tex_coord, 0.0, 1.0);
    vec4 tex_sample = texture(atlas, tex_coord);
    //float attenuation = 1/(1 + (0.007 * distsq_to_light) + (0.002 * distsq_to_light * distsq_to_light));
    FragColor = vec4(tex_sample.xyz * color_mult, tex_sample.w);
    //FragColor = vec4(distsq_to_light, distsq_to_light, distsq_to_light, 1.0);
}
