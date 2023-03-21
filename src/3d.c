#include "3d.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

void
vec3norm(struct vec3* v)
{
    float magnitude = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    if (magnitude == 0) return;
    v->x /= magnitude;
    v->y /= magnitude;
    v->z /= magnitude;
}

struct vec3
vec3cross(struct vec3 a, struct vec3 b)
{
    return (struct vec3){
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x,
    };
}

struct vec3
vec3add(struct vec3 a, struct vec3 b)
{
    return (struct vec3){
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

struct vec3
vec3adds(struct vec3 vector, float scalar)
{
    return (struct vec3){
        vector.x + scalar,
        vector.y + scalar,
        vector.z + scalar,
    };
}

struct vec3
vec3muls(struct vec3 vector, float scalar)
{
    return (struct vec3){
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar,
    };
}

struct vec3
vec3sub(struct vec3 a, struct vec3 b)
{
    return (struct vec3){
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

static float
vec3dot(struct vec3 a, struct vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void
vec3iadd(struct vec3* existing, struct vec3 diff)
{
    existing->x += diff.x;
    existing->y += diff.y;
    existing->z += diff.z;
}

void
vec3isub(struct vec3* existing, struct vec3 diff)
{
    existing->x -= diff.x;
    existing->y -= diff.y;
    existing->z -= diff.z;
}

void
vec3imuls(struct vec3* vector, float scalar)
{
    vector->x *= scalar;
    vector->y *= scalar;
    vector->z *= scalar;
}

struct mat4
projection_matrix(float fovy, float aspect, float near, float far)
{
    const float radians           = fovy * 2.0f * PI / 360.0f;
    const float tangent_half_fovy = tan(radians / 2.0f);

    struct mat4 matrix;
    memset(&matrix, 0, sizeof matrix);

    matrix.values[0][0] = 1.0f / (aspect * tangent_half_fovy);
    matrix.values[1][1] = 1.0f / tangent_half_fovy;
    matrix.values[2][2] = far / (far - near);
    matrix.values[3][2] = -far * near / (far - near);
    matrix.values[2][3] = 1.0f;

    return matrix;
}

struct mat4
view_matrix(struct vec3 eye, struct vec3 direction, struct vec3 up)
{
    struct vec3 forward = direction;
    vec3norm(&forward);
    struct vec3 side = vec3cross(forward, up);
    vec3norm(&side);
    struct vec3 top = vec3cross(forward, side);

    struct mat4 matrix;
    memset(&matrix, 0, sizeof matrix);

    matrix.values[0][0] = side.x;
    matrix.values[1][0] = side.y;
    matrix.values[2][0] = side.z;

    matrix.values[0][1] = top.x;
    matrix.values[1][1] = top.y;
    matrix.values[2][1] = top.z;

    matrix.values[0][2] = forward.x;
    matrix.values[1][2] = forward.y;
    matrix.values[2][2] = forward.z;

    matrix.values[3][0] = -vec3dot(side, eye);
    matrix.values[3][1] = -vec3dot(top, eye);
    matrix.values[3][2] = -vec3dot(forward, eye);

    matrix.values[3][3] = 1.0f;

    return matrix;
}

struct mat4
model_matrix(struct vec3 translation, struct vec3 scale, struct vec3 rotation)
{
    const float cosx = cos(rotation.x);
    const float sinx = sin(rotation.x);
    const float cosy = cos(rotation.y);
    const float siny = sin(rotation.y);
    const float cosz = cos(rotation.z);
    const float sinz = sin(rotation.z);

    struct mat4 matrix;
    memset(&matrix, 0, sizeof matrix);

    matrix.values[0][0] = scale.x * (cosy * cosz + siny * sinx * sinz);
    matrix.values[0][1] = scale.x * (cosx * sinz);
    matrix.values[0][2] = scale.x * (cosy * sinx * sinz - cosz * siny);

    matrix.values[1][0] = scale.y * (cosz * siny * sinx - cosy * sinz);
    matrix.values[1][1] = scale.y * (cosx * cosz);
    matrix.values[1][2] = scale.y * (cosy * cosz * sinz + siny * sinz);

    matrix.values[2][0] = scale.z * (cosx * siny);
    matrix.values[2][1] = scale.z * (-sinx);
    matrix.values[2][2] = scale.z * (cosy * cosx);

    matrix.values[3][0] = translation.x;
    matrix.values[3][0] = translation.y;
    matrix.values[3][0] = translation.z;
    matrix.values[3][3] = 1.0f;

    return matrix;
}
