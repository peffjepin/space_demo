#ifndef PLANET_H
#define PLANET_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "3d.h"
#include "../simplex/simplex.h"

#define PLANET_MAX_SUBDIVISIONS 500
#define PLANET_RADIUS 100.0f

#define NOISE_MIN_GAIN 0.1f
#define NOISE_MIN_FREQUENCY 0.01f
#define NOISE_MIN_LACUNARITY 1.5f
#define NOISE_MIN_SCALE (PLANET_RADIUS / 20.0f)
#define NOISE_MIN_LAYERS 1

#define NOISE_MAX_GAIN 0.9f
#define NOISE_MAX_FREQUENCY 1.0f
#define NOISE_MAX_LACUNARITY 2.5f
#define NOISE_MAX_LAYERS 20
#define NOISE_MAX_SCALE (PLANET_RADIUS / 4.0f)

#define NOISE_INITIAL_GAIN ((NOISE_MIN_GAIN + NOISE_MAX_GAIN) / 2.0f)
#define NOISE_INITIAL_FREQUENCY                                                \
    ((NOISE_MIN_FREQUENCY + NOISE_MAX_FREQUENCY) / 2.0f)
#define NOISE_INITIAL_LACUNARITY                                               \
    ((NOISE_MIN_LACUNARITY + NOISE_MAX_LACUNARITY) / 2.0f)
#define NOISE_INITIAL_LAYERS ((NOISE_MIN_LAYERS + NOISE_MAX_LAYERS) / 2)
#define NOISE_INITIAL_SCALE ((NOISE_MAX_SCALE + NOISE_MIN_SCALE) / 2.0f)

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

Planet             planet_create(uint32_t subdivisions, int seed);
void               planet_destroy(Planet);
struct planet_mesh planet_acquire_mesh(Planet);
void               planet_release_mesh(Planet);
void               planet_set_subdivisions(Planet, uint32_t);
void               planet_set_noise_layers(Planet, uint32_t);
void               planet_set_noise_gain(Planet, float);
void               planet_set_noise_frequency(Planet, float);
void               planet_set_noise_lacunarity(Planet, float);
void               planet_set_noise_scale(Planet, float);
void               planet_set_seed(Planet, int);

#endif  // PLANET_H
