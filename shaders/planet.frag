#version 450

layout (location = 0) in vec3 f_normal;
layout (location = 1) in vec3 f_world_pos;

layout (location = 0) out vec4 frag_color;

void
main() 
{
    const vec3 ambient_light_color = vec3(0.1, 0.1, 0.1);
    const vec3 material_color = vec3(1.0, 0.5, 0.0);
    const vec3 light_direction = normalize(vec3(-1.0, -1.0, -1.0));
    const vec3 light_color = vec3(1.0, 1.0, 1.0);

    float light_strength = max(dot(f_normal, light_direction), 0.0);
    vec3 diffuse_light = light_color * light_strength;
    vec3 final_color = (ambient_light_color + diffuse_light) * material_color;
    frag_color = vec4(final_color, 1.0f);
}
