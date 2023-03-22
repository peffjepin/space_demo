#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec3 f_normal;
layout (location = 1) out vec3 f_color;

layout (binding=0) uniform ubo {
    mat4 model;
    mat4 view;
    mat4 proj;
};

const vec3 DEBUG_COLOR = vec3(0.5, 0.5f, 0.75f);
const vec3 FLAT_GRADE_1_COLOR = vec3(59.0f/255.0f, 93.0f/255.0f, 56.0f/255.0f);
const vec3 FLAT_GRADE_2_COLOR = vec3(148.0f/255.0f, 91.0f/255.0f, 71.0f/255.0f);
const vec3 FLAT_GRADE_3_COLOR = vec3(146.0f/255.0f, 126.0f/255.0f, 119.0f/255.0f);
const vec3 FLAT_GRADE_4_COLOR = vec3(60.0f/255.0f, 66.0f/255.0f, 88.0f/255.0f);

const float GRADE_1_THRESHOLD = 0.9f;
const float GRADE_2_THRESHOLD = 0.8f;
const float GRADE_3_THRESHOLD = 0.7f;
const float GRADE_4_THRESHOLD = 0.6f;

void
main() 
{
    vec4 world_position = model*vec4(position, 1.0f);
    vec4 world_normal = model*vec4(normal, 0.0f);

    // range of [-1, 1] where the closer the value is to 1 the `flatter` the terrain
    // NOTE: values < 0 should not actually ocurr with the current generation algorithm
    float flatness = dot(position, normal) / (length(position) * length(normal));

    if (flatness < 0 ) {
        f_color = DEBUG_COLOR;
    }
    else if (flatness > GRADE_1_THRESHOLD) {
        f_color = FLAT_GRADE_1_COLOR;
    }
    else if (flatness > GRADE_2_THRESHOLD) {
        float t = (1.0f - flatness) / GRADE_2_THRESHOLD;
        f_color = mix(FLAT_GRADE_2_COLOR, FLAT_GRADE_1_COLOR, t);
    }
    else if (flatness > GRADE_3_THRESHOLD) {
        float t = (1.0f - flatness) / GRADE_3_THRESHOLD;
        f_color = mix(FLAT_GRADE_3_COLOR, FLAT_GRADE_2_COLOR, t);
    }
    else if (flatness > GRADE_4_THRESHOLD) {
        float t = (1.0f - flatness) / GRADE_4_THRESHOLD;
        f_color = mix(FLAT_GRADE_3_COLOR, FLAT_GRADE_2_COLOR, t);
    }
    else {
        f_color = FLAT_GRADE_4_COLOR;
    }

    f_normal = normalize(world_normal.xyz);

    gl_Position = proj * view * world_position;
}
