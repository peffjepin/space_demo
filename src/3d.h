#ifndef MATRIX_H
#define MATRIX_H

struct vec3 {
    float x;
    float y;
    float z;
};

struct mat4 {
    float values[4][4];
};

struct mat4 projection_matrix(float fovy, float aspect, float near, float far);
struct mat4 view_matrix(struct vec3 eye, struct vec3 direction, struct vec3 up);

#endif  // MATRIX_H
