#ifndef NOISE_H
#define NOISE_H

#include <math.h>

#include "3d.h"
#include "../simplex/simplex.h"

float terrain_noise(SimplexContext simplex, struct vec3 location);

#endif  // NOISE_H
