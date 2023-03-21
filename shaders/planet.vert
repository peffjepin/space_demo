#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec3 f_normal;
layout (location = 1) out vec3 f_world_pos;

layout (binding=0) uniform ubo {
    mat4 model;
    mat4 view;
    mat4 proj;
};

void
main() 
{
    vec4 world_position = model*vec4(position, 1.0f);
    vec4 world_normal = model*vec4(normal, 0.0f);

    f_normal = normalize(world_normal.xyz);
    f_world_pos = world_position.xyz;

    gl_Position = proj * view * world_position;
}
