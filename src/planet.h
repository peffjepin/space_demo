#ifndef PLANET_H
#define PLANET_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "3d.h"

#define _PLANET_MAX_SUBDIVISIONS 1000

#define PLANET_MAX_INDICES                                                     \
    ((_PLANET_MAX_SUBDIVISIONS - 1) * (_PLANET_MAX_SUBDIVISIONS - 1) * 6)

// unique corners + unique edges + unique faces
#define PLANET_MAX_VERTICES                                                    \
    (8 + 12 * (_PLANET_MAX_SUBDIVISIONS - 1) +                                 \
     (_PLANET_MAX_SUBDIVISIONS - 1) * (_PLANET_MAX_SUBDIVISIONS - 1) * 6)

struct planet {
    uint64_t     id;
    size_t       index_count;
    size_t       vertex_count;
    uint32_t*    indices;
    struct vec3* vertices;
    struct vec3* normals;
};

struct planet* planet_create(uint32_t subdivisions, float radius);
void           planet_destroy(struct planet*);

#endif  // PLANET_H
