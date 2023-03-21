#ifndef PLANET_H
#define PLANET_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "3d.h"
#include "../simplex/simplex.h"

#define PLANET_MAX_SUBDIVISIONS 500
#define PLANET_RADIUS 100.0f

// quads * 2 triangles per quad * 3 indices per triangle * 6 faces per cube
#define PLANET_MAX_INDICES                                                     \
    (PLANET_MAX_SUBDIVISIONS * PLANET_MAX_SUBDIVISIONS * 2 * 3 * 6)

// allow for repeat edge/corner vertices because stitching them together
// seems beyond the scope of a simple demo
#define PLANET_MAX_VERTICES                                                    \
    ((PLANET_MAX_SUBDIVISIONS + 1) * (PLANET_MAX_SUBDIVISIONS + 1) * 6)

typedef struct planet* Planet;

struct planet_mesh {
    uint64_t     iteration;
    size_t       vertex_count;
    size_t       index_count;
    struct vec3* vertices;
    struct vec3* normals;
    uint32_t*    indices;
};

Planet             planet_create(uint32_t subdivisions);
void               planet_destroy(Planet);
struct planet_mesh planet_acquire_mesh(Planet);
void               planet_release_mesh(Planet);
void               planet_set_subdivisions(Planet, uint32_t);

#endif  // PLANET_H
