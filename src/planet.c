#include "planet.h"

#include <stdio.h>
#include <stdlib.h>

static void
construct_subdivided_cube(
    struct planet* planet, uint32_t subdivisions, float scale
)
{
    static const struct vec3 vec3zero = {0};

    planet->vertex_count = 0;
    planet->index_count  = 0;

    const float half_scale = scale / 2.0f;

    for (size_t y = 0; y < subdivisions + 1; y++) {
        for (size_t x = 0; x < subdivisions + 1; x++) {
            struct vec3 vertex = {
                -half_scale + ((float)x / subdivisions * scale),
                -half_scale + ((float)y / subdivisions * scale),
                0.0f,
            };
            planet->vertices[planet->vertex_count] = vertex;
            planet->normals[planet->vertex_count]  = vec3zero;
            planet->vertex_count += 1;
        }
    }

    for (size_t y = 0; y < subdivisions; y++) {
        for (size_t x = 0; x < subdivisions; x++) {
            planet->indices[planet->index_count++] = y * (subdivisions + 1) + x;
            planet->indices[planet->index_count++] =
                y * (subdivisions + 1) + x + 1;
            planet->indices[planet->index_count++] =
                (y + 1) * (subdivisions + 1) + x;

            planet->indices[planet->index_count++] =
                y * (subdivisions + 1) + x + 1;
            planet->indices[planet->index_count++] =
                (y + 1) * (subdivisions + 1) + x + 1;
            planet->indices[planet->index_count++] =
                (y + 1) * (subdivisions + 1) + x;
        }
    }
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

    planet->vertices = calloc(PLANET_MAX_VERTICES, sizeof *planet->vertices);
    planet->normals  = calloc(PLANET_MAX_VERTICES, sizeof *planet->normals);
    planet->indices  = calloc(PLANET_MAX_VERTICES, sizeof *planet->indices);
    if (planet->vertices == NULL || planet->normals == NULL ||
        planet->indices == NULL) {
        fprintf(stderr, "ERROR: failed to allocate planet\n");
        exit(EXIT_FAILURE);
    }

    planet->id = 1;
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
