#include "planet.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void
construct_subdivided_face(
    struct planet* planet,
    uint32_t       subdivisions,
    float          radius,
    size_t         start_vertex,
    size_t         start_index,
    struct vec3    corner,
    struct vec3    dx,
    struct vec3    dy
)
{
    static const struct vec3 vec3zero = {0};

    // construct vertices
    for (size_t y = 0; y < subdivisions + 1; y++) {
        for (size_t x = 0; x < subdivisions + 1; x++) {
            struct vec3 vertex = corner;
            vertex.x += dx.x * (float)x;
            vertex.y += dx.y * (float)x;
            vertex.z += dx.z * (float)x;
            vertex.x += dy.x * (float)y;
            vertex.y += dy.y * (float)y;
            vertex.z += dy.z * (float)y;
            vec3norm(&vertex);
            vertex.x *= radius;
            vertex.y *= radius;
            vertex.z *= radius;

            size_t local_vertex_index = y * (subdivisions + 1) + x;
            planet->vertices[start_vertex + local_vertex_index] = vertex;
            planet->normals[start_vertex + local_vertex_index]  = vec3zero;
        }
    }

    // accumulate normals and construct indices
    for (size_t y = 0; y < subdivisions; y++) {
        for (size_t x = 0; x < subdivisions; x++) {
            // vertex indices
            size_t vi1 = start_vertex + y * (subdivisions + 1) + x;
            size_t vi2 = start_vertex + y * (subdivisions + 1) + x + 1;
            size_t vi3 = start_vertex + (y + 1) * (subdivisions + 1) + x;

            // vertex values
            struct vec3 vert1 = planet->vertices[vi1];
            struct vec3 vert2 = planet->vertices[vi2];
            struct vec3 vert3 = planet->vertices[vi3];

            // accumulate normals
            struct vec3 edge1 = vert3;
            edge1.x -= vert1.x;
            edge1.y -= vert1.y;
            edge1.z -= vert1.z;
            struct vec3 edge2 = vert2;
            edge2.x -= vert1.x;
            edge2.y -= vert1.y;
            edge2.z -= vert1.z;
            struct vec3 normal = vec3cross(edge1, edge2);
            planet->normals[vi1].x += normal.x;
            planet->normals[vi1].y += normal.y;
            planet->normals[vi1].z += normal.z;
            planet->normals[vi2].x += normal.x;
            planet->normals[vi2].y += normal.y;
            planet->normals[vi2].z += normal.z;
            planet->normals[vi3].x += normal.x;
            planet->normals[vi3].y += normal.y;
            planet->normals[vi3].z += normal.z;

            // construct indices
            planet->indices[start_index++] = vi1;
            planet->indices[start_index++] = vi2;
            planet->indices[start_index++] = vi3;

            // vertex indices
            vi1 = start_vertex + y * (subdivisions + 1) + x + 1;
            vi2 = start_vertex + (y + 1) * (subdivisions + 1) + x + 1;
            vi3 = start_vertex + (y + 1) * (subdivisions + 1) + x;

            // vertex values
            vert1 = planet->vertices[vi1];
            vert2 = planet->vertices[vi2];
            vert3 = planet->vertices[vi3];

            // accumulate normals
            edge1 = vert3;
            edge1.x -= vert1.x;
            edge1.y -= vert1.y;
            edge1.z -= vert1.z;
            edge2 = vert2;
            edge2.x -= vert1.x;
            edge2.y -= vert1.y;
            edge2.z -= vert1.z;
            normal = vec3cross(edge1, edge2);
            planet->normals[vi1].x += normal.x;
            planet->normals[vi1].y += normal.y;
            planet->normals[vi1].z += normal.z;
            planet->normals[vi2].x += normal.x;
            planet->normals[vi2].y += normal.y;
            planet->normals[vi2].z += normal.z;
            planet->normals[vi3].x += normal.x;
            planet->normals[vi3].y += normal.y;
            planet->normals[vi3].z += normal.z;

            // construct indices
            planet->indices[start_index++] = vi1;
            planet->indices[start_index++] = vi2;
            planet->indices[start_index++] = vi3;
        }
    }

    // normalize accumulated normals
    for (size_t i = 0; i < (subdivisions + 1) * (subdivisions + 1); i++) {
        vec3norm(planet->normals + i);
    }
}

static void
construct_subdivided_cube(
    struct planet* planet, uint32_t subdivisions, float scale
)
{
    const float  half_scale        = scale / 2.0f;
    const float  interval          = scale / (float)subdivisions;
    const size_t vertices_per_face = (subdivisions + 1) * (subdivisions + 1);
    const size_t indices_per_face  = subdivisions * subdivisions * 2 * 3;

    // front
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        0 * vertices_per_face,
        0 * indices_per_face,
        (struct vec3){-half_scale, -half_scale, -half_scale},
        (struct vec3){interval, 0.0f, 0.0f},
        (struct vec3){0.0f, interval, 0.0f}
    );
    // left
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        1 * vertices_per_face,
        1 * indices_per_face,
        (struct vec3){-half_scale, -half_scale, half_scale},
        (struct vec3){0.0f, 0.0f, -interval},
        (struct vec3){0.0f, interval, 0.0f}
    );
    // back
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        2 * vertices_per_face,
        2 * indices_per_face,
        (struct vec3){half_scale, -half_scale, half_scale},
        (struct vec3){-interval, 0.0f, 0.0f},
        (struct vec3){0.0f, interval, 0.0f}
    );
    // right
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        3 * vertices_per_face,
        3 * indices_per_face,
        (struct vec3){half_scale, -half_scale, -half_scale},
        (struct vec3){0.0f, 0.0f, interval},
        (struct vec3){0.0f, interval, 0.0f}
    );
    // top
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        4 * vertices_per_face,
        4 * indices_per_face,
        (struct vec3){-half_scale, -half_scale, half_scale},
        (struct vec3){interval, 0.0f, 0.0f},
        (struct vec3){0.0f, 0.0f, -interval}
    );
    // bottom
    construct_subdivided_face(
        planet,
        subdivisions,
        scale,
        5 * vertices_per_face,
        5 * indices_per_face,
        (struct vec3){-half_scale, half_scale, -half_scale},
        (struct vec3){interval, 0.0f, 0.0f},
        (struct vec3){0.0f, 0.0f, interval}
    );
}

struct planet*
planet_create(uint32_t subdivisions, float radius)
{
    if (subdivisions == 0 || subdivisions > _PLANET_MAX_SUBDIVISIONS) {
        fprintf(stderr, "ERROR: max planet vertices exceeded\n");
        exit(EXIT_FAILURE);
    }
    struct planet* planet;
    planet = calloc(1, sizeof *planet);
    if (planet == NULL) {
        fprintf(stderr, "ERROR: failed to allocate planet\n");
        exit(EXIT_FAILURE);
    }

    planet->vertex_count = (subdivisions + 1) * (subdivisions + 1) * 6;
    planet->index_count  = (subdivisions) * (subdivisions)*2 * 3 * 6;
    planet->id           = 1;
    assert(planet->vertex_count < PLANET_MAX_VERTICES);
    assert(planet->index_count < PLANET_MAX_INDICES);

    planet->vertices = calloc(PLANET_MAX_VERTICES, sizeof *planet->vertices);
    planet->normals  = calloc(PLANET_MAX_VERTICES, sizeof *planet->normals);
    planet->indices  = calloc(PLANET_MAX_VERTICES, sizeof *planet->indices);

    if (planet->vertices == NULL || planet->normals == NULL ||
        planet->indices == NULL) {
        fprintf(stderr, "ERROR: failed to allocate planet\n");
        exit(EXIT_FAILURE);
    }

    construct_subdivided_cube(planet, subdivisions, radius);

    return planet;
}

void
planet_destroy(struct planet* planet)
{
    free(planet->vertices);
    free(planet->indices);
    free(planet->normals);
    free(planet);
}
