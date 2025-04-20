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

    vec3 normal = vec3(0, 0, 1);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
    //vec4 shadow_sample = texture(shadow_map, uv);
    //float shadow_val = shadow_sample.r;

    vec3 player_light_pos = point_lights[0];
    player_light_pos.z = 5;
    vec3 view_dir = normalize(player_light_pos - frag);

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
        float r = 161.0 / 255.0;
        float g = 150.0 / 255.0;
        light_color.x = r;// + (light_index * (1 - r));
        light_color.y = g;// + (light_index * (1 - g));
        light_color.z = max(0.3, light_index);
        light.z = 5.0 + 30*light_index;

        float distsq_to_light = length(light - frag);//abs(dot(light - frag, light - frag));
        vec3 reflect_dir = normalize(reflect(-light, normal));

        float diffuse = max(dot(light, normal), 0);
        float specish = pow(max(dot(view_dir, reflect_dir), 0), 32);
        float attenuation = (1 - shadow_avg) *
            (2.0 / (1.0 + (0.022*distsq_to_light + (0.019 * distsq_to_light * distsq_to_light))));

        vec3 color = diffuse * light_color * tex_sample.xyz * attenuation;
        //color += specish * light_color * attenuation;
        color += 0.1 * tex_sample.xyz; // ambient

        //color_mult = diffuse * attenuation;
        //color_mult += 0.2; // ambient light
        final_color += 0.5 * vec4(color, tex_sample.w);//vec4(final_color.xyz + (0.5 * light_color * tex_sample.xyz * color_mult), final_color.w);
    }

    FragColor = final_color;
    //FragColor = vec4(shadow_val, 0, 0, 1);
    //FragColor = vec4(distsq_to_light, distsq_to_light, distsq_to_light, 1.0);
}
