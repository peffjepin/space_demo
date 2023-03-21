#include "planet.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../simplex/simplex.h"

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
            // assumes the caller is responsible for centering the cube about
            // (0,0,0)
            vec3norm(&vertex);
            float noise =
                simplex_sample3(planet->simplex, vertex.x, vertex.y, vertex.z);
            vertex.x *= (radius + noise);
            vertex.y *= (radius + noise);
            vertex.z *= (radius + noise);

            size_t local_vertex_index = y * (subdivisions + 1) + x;
            planet->vertices[start_vertex + local_vertex_index] = vertex;
            planet->normals[start_vertex + local_vertex_index]  = vec3zero;
        }
    }

    // clang-format off
    // accumulate normals and construct indices
    for (size_t y = 0; y < subdivisions; y++) {
        for (size_t x = 0; x < subdivisions; x++) {
            // first triangle
            //
            // indices
            size_t vertex_index_1 = start_vertex + y * (subdivisions + 1) + x;
            size_t vertex_index_2 = start_vertex + y * (subdivisions + 1) + x + 1;
            size_t vertex_index_3 = start_vertex + (y + 1) * (subdivisions + 1) + x;
            planet->indices[start_index++] = vertex_index_1;
            planet->indices[start_index++] = vertex_index_2;
            planet->indices[start_index++] = vertex_index_3;

            // normals
            struct vec3 vertex_1 = planet->vertices[vertex_index_1];
            struct vec3 vertex_2 = planet->vertices[vertex_index_2];
            struct vec3 vertex_3 = planet->vertices[vertex_index_3];

            struct vec3 edge1 = vec3sub(vertex_3, vertex_1);
            struct vec3 edge2 = vec3sub(vertex_2, vertex_1);
            struct vec3 normal = vec3cross(edge1, edge2);

            vec3iadd(planet->normals+vertex_index_1, normal);
            vec3iadd(planet->normals+vertex_index_2, normal);
            vec3iadd(planet->normals+vertex_index_3, normal);

            // second triangle
            //
            // indices
            vertex_index_1 = start_vertex + y * (subdivisions + 1) + x + 1;
            vertex_index_2 = start_vertex + (y + 1) * (subdivisions + 1) + x + 1;
            vertex_index_3 = start_vertex + (y + 1) * (subdivisions + 1) + x;
            planet->indices[start_index++] = vertex_index_1;
            planet->indices[start_index++] = vertex_index_2;
            planet->indices[start_index++] = vertex_index_3;

            // normals
            vertex_1 = planet->vertices[vertex_index_1];
            vertex_2 = planet->vertices[vertex_index_2];
            vertex_3 = planet->vertices[vertex_index_3];
            edge1 = vec3sub(vertex_3, vertex_1);
            edge2 = vec3sub(vertex_2, vertex_1);
            normal = vec3cross(edge1, edge2);
            vec3iadd(planet->normals+vertex_index_1, normal);
            vec3iadd(planet->normals+vertex_index_2, normal);
            vec3iadd(planet->normals+vertex_index_3, normal);
        }
    }
    // clang-format on

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
    planet->simplex      = simplex_context_create(0);
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
    if (planet == NULL) return;
    simplex_context_destroy(planet->simplex);
    free(planet->vertices);
    free(planet->indices);
    free(planet->normals);
    free(planet);
}
