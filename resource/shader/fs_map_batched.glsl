#version 400 core

in vec2 tex_coord;
in vec3 frag_world_pos;

out vec4 FragColor;

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    vec3 point_lights[24];
};

uniform sampler2D atlas;
uniform sampler2DArray shadow_map;

void main()
{
    float color_mult = 0;
    vec3 frag = frag_world_pos;

    //vec4 shadow_sample = texture(shadow_map, uv);
    //float shadow_val = shadow_sample.r;

    vec4 tex_sample = texture(atlas, tex_coord);
    vec4 final_color = vec4(0, 0, 0, tex_sample.w);
    for (int light_index = 0; light_index < 2; light_index++) {
        float shadow_avg = 0;
        for (int voffset = -1; voffset < 2; voffset++) {
            for (int hoffset = -1; hoffset < 2; hoffset++) {
                vec3 uv = vec3((frag.x + hoffset) / 320.0, (frag.y + voffset) / 180.0, light_index);
                vec4 shadow_sample = texture(shadow_map, uv);
                shadow_avg += shadow_sample.r;
            }
        }

        shadow_avg = shadow_avg / 9.0;

        vec3 light = point_lights[light_index];
        float distsq_to_light = length(light - frag);//abs(dot(light - frag, light - frag));
        float attenuation = (1 - shadow_avg) *
            (2.0 / (1.0 + (0.022*distsq_to_light + (0.0019 * distsq_to_light * distsq_to_light))));
        color_mult = attenuation;
        color_mult += 0.2; // ambient light
        final_color = vec4(final_color.xyz + (0.5 * tex_sample.xyz * color_mult), final_color.w);
    }

    FragColor = final_color;
    //FragColor = vec4(shadow_val, 0, 0, 1);
    //FragColor = vec4(distsq_to_light, distsq_to_light, distsq_to_light, 1.0);
}
