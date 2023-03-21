#ifndef MATRIX_H
#define MATRIX_H

struct vec3 {
    float x;
    float y;
    float z;
};

struct vec3 vec3cross(struct vec3, struct vec3);
void        vec3norm(struct vec3*);

struct mat4 {
    float values[4][4];
};

struct mat4 model_matrix(
    struct vec3 translation, struct vec3 scale, struct vec3 rotation
);
struct mat4 projection_matrix(float fovy, float aspect, float near, float far);
struct mat4 view_matrix(struct vec3 eye, struct vec3 direction, struct vec3 up);

#endif  // MATRIX_H
