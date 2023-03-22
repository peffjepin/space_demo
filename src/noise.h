#ifndef NOISE_H
#define NOISE_H

#include <math.h>

#include "3d.h"
#include "../simplex/simplex.h"

float terrain_noise(
    SimplexContext simplex,
    struct vec3    location,
    uint32_t       layers,
    float          gain,
    float          frequency,
    float          lacunarity
);

#endif  // NOISE_H
