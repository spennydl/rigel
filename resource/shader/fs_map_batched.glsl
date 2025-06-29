#version 400 core

in vec2 tex_coord;
in vec3 frag_world_pos;

out vec4 FragColor;

struct Light
{
    vec4 position;
    vec4 color;
    vec4 data;
};

layout (std140) uniform GlobalUniforms {
    mat4 screen;
    Light point_lights[24];
    Light circle_lights[24];
    vec4 n_lights;
};

uniform sampler2D atlas;
uniform sampler2DArray shadow_map;
uniform float dimmer;

void main()
{
    float color_mult = 0;
    vec3 frag = frag_world_pos;

    vec3 normal = vec3(0, 0, 1);
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec4 tex_sample = texture(atlas, tex_coord);
    vec3 final_color = vec3(0);

    float num_lights = n_lights.x;

    // point lights
    for (int light_index = 0; light_index < num_lights; light_index++) {
        float shadow_avg = 0;
        for (int voffset = -1; voffset < 2; voffset++) {
            for (int hoffset = -1; hoffset < 2; hoffset++) {
                vec3 uv = vec3((frag.x + hoffset) / 320.0, (frag.y + voffset) / 180.0, light_index);
                vec4 shadow_sample = texture(shadow_map, uv);
                shadow_avg += shadow_sample.r;
            }
        }

        shadow_avg = shadow_avg / 9.0;

        Light light = point_lights[light_index];
        vec3 light_position = light.position.xyz;

        float distsq_to_light = length(light_position - frag);
        vec3 light_dir = normalize(light_position - frag);

        float cos_t = max(dot(light_dir, normal), 0);
        vec3 diffuse = tex_sample.xyz / 3.14159;
        float attenuation = (1 - shadow_avg)
              * (2.0 / (1.0 + (0.07*distsq_to_light + (0.017 * distsq_to_light * distsq_to_light))));

        vec3 color = diffuse * cos_t * light.color.xyz * attenuation;

        final_color += color;
    }

    final_color += pow(0.1, 2.2) * tex_sample.xyz;//mix(tex_sample.xyz, vec3(0.0, 0.0, 0.4), 0.1); // ambient
    FragColor = dimmer * vec4(final_color, tex_sample.w);
}
