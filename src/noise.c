#include "noise.h"

float
fbm(SimplexContext simplex,
    struct vec3    location,
    uint32_t       layers,
    float          gain,
    float          freq,
    float          lacunarity)
{
    float total  = 0.0f;
    float factor = 1.0f;
    for (unsigned int layer = 0; layer < layers; layer++) {
        vec3imuls(&location, freq);
        float noise =
            (float)simplex_sample3(simplex, location.x, location.y, location.z);
        total += noise * factor;
        factor *= gain;
        freq *= lacunarity;
    }
    return total;
}

float
terrain_noise(
    SimplexContext simplex,
    struct vec3    location,
    uint32_t       layers,
    float          gain,
    float          frequency,
    float          lacunarity
)
{
    float height = fbm(simplex, location, layers, gain, frequency, lacunarity);
    return height;
}
