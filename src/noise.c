#include "noise.h"

float
fbm(SimplexContext simplex,
    struct vec3    location,
    unsigned int   layers,
    float          gain,
    float          freq)
{
    float total  = 0.0f;
    float factor = 0.5f;
    for (unsigned int layer = 0; layer < layers; layer++) {
        vec3imuls(&location, freq);
        float noise =
            simplex_sample3(simplex, location.x, location.y, location.z);
        total += noise * factor;
        factor *= gain;
        freq *= 2.0f;
    }
    return total;
}

float
terrain_noise(SimplexContext simplex, struct vec3 location)
{
    static const float HEIGHT_SCALE = 10.0f;

    int   layers = 10;
    float gain   = 0.5f;
    float freq   = 0.25f;

    float height = fbm(simplex, location, layers, gain, freq);

    return height * HEIGHT_SCALE;
}
