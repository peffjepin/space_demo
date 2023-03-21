#ifndef MATRIX_H
#define MATRIX_H

struct vec3 {
    float x;
    float y;
    float z;
};

struct vec3 vec3cross(struct vec3, struct vec3);
struct vec3 vec3add(struct vec3, struct vec3);
struct vec3 vec3sub(struct vec3, struct vec3);
struct vec3 vec3adds(struct vec3, float scalar);
struct vec3 vec3muls(struct vec3, float scalar);

// in place operations
void vec3iadd(struct vec3*, struct vec3);
void vec3isub(struct vec3*, struct vec3);
void vec3imuls(struct vec3*, float scalar);
void vec3norm(struct vec3*);

struct mat4 {
    float values[4][4];
};

struct mat4 model_matrix(
    struct vec3 translation, struct vec3 scale, struct vec3 rotation
);
struct mat4 projection_matrix(float fovy, float aspect, float near, float far);
struct mat4 view_matrix(struct vec3 eye, struct vec3 direction, struct vec3 up);

#endif  // MATRIX_H
