#include "planet.h"

// Not exactly sure why this isn't working for me 
// with cl.exe. As far as I can tell it's supposed to be
// supported and the header exists but including it spews
// a ton of syntax errors for me. A quick check online yielded
// no answers so I guess I'll just hack around it as much
// as that sucks.
#ifndef _WIN32
#include <stdatomic.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "noise.h"

struct generation_params {
    uint32_t subdivisions;
    uint32_t noise_layers;
    float    noise_gain;
    float    noise_frequency;
    float    noise_lacunarity;
    float    noise_scale;
};

static bool
generation_params_equal(
    struct generation_params* p1, struct generation_params* p2
)
{
    return memcmp(p1, p2, sizeof *p1) == 0;
}

struct planet {
    SDL_mutex*  mutex;
    SDL_Thread* thread;
#ifndef _WIN32
    atomic_int  shutdown_signal;
#else
    int shutdown_signal;
#endif

    // used only by the generator thread, no sync required
    SimplexContext           simplex;
    uint32_t*                generator_indices;
    struct vec3*             generator_vertices;
    struct vec3*             generator_normals;
    struct generation_params generated_params;

    // available to the main thread, sync required
    uint64_t                 id;
    struct generation_params configured_params;
    uint32_t                 index_count;
    uint32_t                 vertex_count;
    uint32_t*                indices;
    struct vec3*             vertices;
    struct vec3*             normals;
};

static bool check_shutdown_signal(struct planet* planet) {
    bool result = false;
#ifdef _WIN32
    SDL_LockMutex(planet->mutex);
    result = planet->shutdown_signal != 0;
    SDL_UnlockMutex(planet->mutex);
#else
    result = atomic_load(&planet->shutdown_signal) != 0;
#endif
    return result;
}

static void set_shutdown_signal(struct planet* planet) {
#ifdef _WIN32
    SDL_LockMutex(planet->mutex);
    planet->shutdown_signal = 1;
    SDL_UnlockMutex(planet->mutex);
#else
    atomic_store(&planet->shutdown_signal, 1);
#endif
}

struct face_generation_context {
    struct planet*            planet;
    struct generation_params* params;
    uint32_t                  start_vertex;
    uint32_t                  start_index;
    struct vec3               corner;
    struct vec3               dx;
    struct vec3               dy;
};

// if subdivisions have not changed we can avoid regenerating the geometry and
// just recalculate vertex positions/normals
static int
regenerate_face(struct face_generation_context* ctx)
{
    static const struct vec3 vec3zero = {0.0f, 0.0f, 0.0f};

    const uint32_t vertices_per_face =
        (ctx->params->subdivisions + 1) * (ctx->params->subdivisions + 1);
    const uint32_t indices_per_face =
        ctx->params->subdivisions * ctx->params->subdivisions * 2 * 3;

    for (uint32_t i = ctx->start_vertex;
         i < ctx->start_vertex + vertices_per_face;
         i++) {
        ctx->planet->generator_normals[i] = vec3zero;

        // we're safe to touch these without sync as long as we're only reading
        struct vec3 vertex = ctx->planet->vertices[i];
        vec3norm(&vertex);
        float noise = terrain_noise(
            ctx->planet->simplex,
            vertex,
            ctx->params->noise_layers,
            ctx->params->noise_gain,
            ctx->params->noise_frequency,
            ctx->params->noise_lacunarity
        );
        vec3imuls(&vertex, PLANET_RADIUS + noise * ctx->params->noise_scale);
        ctx->planet->generator_vertices[i] = vertex;
    }

    for (uint32_t i = ctx->start_index; i < ctx->start_index + indices_per_face;
         i += 3) {
        const uint32_t index_1 = ctx->planet->indices[i + 0];
        const uint32_t index_2 = ctx->planet->indices[i + 1];
        const uint32_t index_3 = ctx->planet->indices[i + 2];

        // set indices because the generator buffer might not have these indices
        // in it yet
        ctx->planet->generator_indices[i + 0] = index_1;
        ctx->planet->generator_indices[i + 1] = index_2;
        ctx->planet->generator_indices[i + 2] = index_3;

        struct vec3 vertex_1 = ctx->planet->generator_vertices[index_1];
        struct vec3 vertex_2 = ctx->planet->generator_vertices[index_2];
        struct vec3 vertex_3 = ctx->planet->generator_vertices[index_3];

        struct vec3 edge1  = vec3sub(vertex_3, vertex_1);
        struct vec3 edge2  = vec3sub(vertex_2, vertex_1);
        struct vec3 normal = vec3cross(edge1, edge2);

        vec3iadd(ctx->planet->generator_normals + index_1, normal);
        vec3iadd(ctx->planet->generator_normals + index_2, normal);
        vec3iadd(ctx->planet->generator_normals + index_3, normal);
    }

    for (uint32_t i = ctx->start_vertex;
         i < ctx->start_vertex + vertices_per_face;
         i++) {
        vec3norm(ctx->planet->generator_normals + i);
    }

    return 0;
}

static int
construct_subdivided_face(struct face_generation_context* ctx)
{
    static const struct vec3 vec3zero = {0.0f, 0.0f, 0.0f};

    // construct vertices
    for (uint32_t y = 0; y < ctx->params->subdivisions + 1; y++) {
        for (uint32_t x = 0; x < ctx->params->subdivisions + 1; x++) {
            struct vec3 vertex = ctx->corner;
            vertex.x += ctx->dx.x * (float)x;
            vertex.y += ctx->dx.y * (float)x;
            vertex.z += ctx->dx.z * (float)x;
            vertex.x += ctx->dy.x * (float)y;
            vertex.y += ctx->dy.y * (float)y;
            vertex.z += ctx->dy.z * (float)y;
            // assumes the caller is responsible for centering the cube about
            // (0,0,0)
            vec3norm(&vertex);
            float noise = terrain_noise(
                ctx->planet->simplex,
                vertex,
                ctx->params->noise_layers,
                ctx->params->noise_gain,
                ctx->params->noise_frequency,
                ctx->params->noise_lacunarity
            );
            vec3imuls(
                &vertex, PLANET_RADIUS + noise * ctx->params->noise_scale
            );

            uint32_t vertex_index =
                ctx->start_vertex + y * (ctx->params->subdivisions + 1) + x;
            ctx->planet->generator_vertices[vertex_index] = vertex;
            ctx->planet->generator_normals[vertex_index]  = vec3zero;
        }
    }

    // clang-format off
    // accumulate normals and construct indices
    for (uint32_t y = 0; y < ctx->params->subdivisions; y++) {
        for (uint32_t x = 0; x < ctx->params->subdivisions; x++) {
            // first triangle
            //
            // indices
            uint32_t vertex_index_1 = ctx->start_vertex + y * (ctx->params->subdivisions + 1) + x;
            uint32_t vertex_index_2 = ctx->start_vertex + y * (ctx->params->subdivisions + 1) + x + 1;
            uint32_t vertex_index_3 = ctx->start_vertex + (y + 1) * (ctx->params->subdivisions + 1) + x;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_1;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_2;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_3;

            // normals
            struct vec3 vertex_1 = ctx->planet->generator_vertices[vertex_index_1];
            struct vec3 vertex_2 = ctx->planet->generator_vertices[vertex_index_2];
            struct vec3 vertex_3 = ctx->planet->generator_vertices[vertex_index_3];

            struct vec3 edge1 = vec3sub(vertex_3, vertex_1);
            struct vec3 edge2 = vec3sub(vertex_2, vertex_1);
            struct vec3 normal = vec3cross(edge1, edge2);

            vec3iadd(ctx->planet->generator_normals+vertex_index_1, normal);
            vec3iadd(ctx->planet->generator_normals+vertex_index_2, normal);
            vec3iadd(ctx->planet->generator_normals+vertex_index_3, normal);

            // second triangle
            //
            // indices
            vertex_index_1 = ctx->start_vertex + y * (ctx->params->subdivisions + 1) + x + 1;
            vertex_index_2 = ctx->start_vertex + (y + 1) * (ctx->params->subdivisions + 1) + x + 1;
            vertex_index_3 = ctx->start_vertex + (y + 1) * (ctx->params->subdivisions + 1) + x;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_1;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_2;
            ctx->planet->generator_indices[ctx->start_index++] = vertex_index_3;

            // normals
            vertex_1 = ctx->planet->generator_vertices[vertex_index_1];
            vertex_2 = ctx->planet->generator_vertices[vertex_index_2];
            vertex_3 = ctx->planet->generator_vertices[vertex_index_3];
            edge1 = vec3sub(vertex_3, vertex_1);
            edge2 = vec3sub(vertex_2, vertex_1);
            normal = vec3cross(edge1, edge2);
            vec3iadd(ctx->planet->generator_normals+vertex_index_1, normal);
            vec3iadd(ctx->planet->generator_normals+vertex_index_2, normal);
            vec3iadd(ctx->planet->generator_normals+vertex_index_3, normal);
        }
    }
    // clang-format on

    // normalize accumulated normals
    for (uint32_t i = 0;
         i < (ctx->params->subdivisions + 1) * (ctx->params->subdivisions + 1);
         i++) {
        vec3norm(ctx->planet->generator_normals + i);
    }

    return 0;
}

static void
construct_subdivided_cube(
    struct planet*            planet,
    struct generation_params* params,
    bool                      generate_geometry
)
{
    static const float half_scale = PLANET_RADIUS / 2.0f;
    const float        interval   = PLANET_RADIUS / (float)params->subdivisions;

    const uint32_t vertices_per_face =
        (params->subdivisions + 1) * (params->subdivisions + 1);
    const uint32_t indices_per_face =
        params->subdivisions * params->subdivisions * 2 * 3;

    SDL_ThreadFunction thread_main =
        (generate_geometry) ? (SDL_ThreadFunction)construct_subdivided_face
                            : (SDL_ThreadFunction)regenerate_face;
    SDL_Thread* threads[6];

    struct face_generation_context front_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 0 * vertices_per_face,
        .start_index  = 0 * indices_per_face,
        .corner       = (struct vec3){-half_scale, -half_scale, -half_scale},
        .dx           = (struct vec3){interval, 0.0f, 0.0f},
        .dy           = (struct vec3){0.0f, interval, 0.0f},
    };
    threads[0] =
        SDL_CreateThread(thread_main, "front face thread", &front_face_context);

    struct face_generation_context left_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 1 * vertices_per_face,
        .start_index  = 1 * indices_per_face,
        .corner       = (struct vec3){-half_scale, -half_scale, half_scale},
        .dx           = (struct vec3){0.0f, 0.0f, -interval},
        .dy           = (struct vec3){0.0f, interval, 0.0f},
    };
    threads[1] =
        SDL_CreateThread(thread_main, "left face thread", &left_face_context);

    struct face_generation_context back_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 2 * vertices_per_face,
        .start_index  = 2 * indices_per_face,
        .corner       = (struct vec3){half_scale, -half_scale, half_scale},
        .dx           = (struct vec3){-interval, 0.0f, 0.0f},
        .dy           = (struct vec3){0.0f, interval, 0.0f},
    };
    threads[2] =
        SDL_CreateThread(thread_main, "back face thread", &back_face_context);

    struct face_generation_context right_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 3 * vertices_per_face,
        .start_index  = 3 * indices_per_face,
        .corner       = (struct vec3){half_scale, -half_scale, -half_scale},
        .dx           = (struct vec3){0.0f, 0.0f, interval},
        .dy           = (struct vec3){0.0f, interval, 0.0f},
    };
    threads[3] =
        SDL_CreateThread(thread_main, "right face thread", &right_face_context);

    struct face_generation_context top_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 4 * vertices_per_face,
        .start_index  = 4 * indices_per_face,
        .corner       = (struct vec3){-half_scale, -half_scale, half_scale},
        .dx           = (struct vec3){interval, 0.0f, 0.0f},
        .dy           = (struct vec3){0.0f, 0.0f, -interval},
    };
    threads[4] =
        SDL_CreateThread(thread_main, "top face thread", &top_face_context);

    struct face_generation_context bottom_face_context = {
        .planet       = planet,
        .params       = params,
        .start_vertex = 5 * vertices_per_face,
        .start_index  = 5 * indices_per_face,
        .corner       = (struct vec3){-half_scale, half_scale, -half_scale},
        .dx           = (struct vec3){interval, 0.0f, 0.0f},
        .dy           = (struct vec3){0.0f, 0.0f, interval},
    };
    threads[5] = SDL_CreateThread(
        thread_main, "bottom face thread", &bottom_face_context
    );

    for (uint32_t i = 0; i < 6; i++) {
        SDL_WaitThread(threads[i], NULL);
    }
}

static int
planet_generation_main(struct planet* planet)
{
    while (!check_shutdown_signal(planet)) {
        SDL_LockMutex(planet->mutex);
        struct generation_params configured = planet->configured_params;
        bool                     requires_regeneration =
            !generation_params_equal(&configured, &planet->generated_params);
        SDL_UnlockMutex(planet->mutex);

        if (requires_regeneration) {
            uint32_t vertex_count = (configured.subdivisions + 1) *
                                  (configured.subdivisions + 1) * 6;
            uint32_t index_count = (configured.subdivisions) *
                                 (configured.subdivisions) * 2 * 3 * 6;
            assert(vertex_count <= PLANET_MAX_VERTICES);
            assert(index_count <= PLANET_MAX_INDICES);

            construct_subdivided_cube(
                planet,
                &configured,
                planet->generated_params.subdivisions != configured.subdivisions
            );

            SDL_LockMutex(planet->mutex);

            struct vec3* current_vertices = planet->vertices;
            struct vec3* current_normals  = planet->normals;
            uint32_t*    current_indices  = planet->indices;
            planet->indices               = planet->generator_indices;
            planet->normals               = planet->generator_normals;
            planet->vertices              = planet->generator_vertices;
            planet->generator_indices     = current_indices;
            planet->generator_normals     = current_normals;
            planet->generator_vertices    = current_vertices;
            planet->vertex_count          = vertex_count;
            planet->index_count           = index_count;
            planet->generated_params      = configured;
            planet->id++;

            SDL_UnlockMutex(planet->mutex);
        }

        if (check_shutdown_signal(planet)) break;
        SDL_Delay(1);
    }
    return 0;
}

struct planet*
planet_create(uint32_t subdivisions)
{
    if (subdivisions == 0 || subdivisions > PLANET_MAX_SUBDIVISIONS) {
        fprintf(stderr, "ERROR: max planet subdivisions exceeded\n");
        exit(EXIT_FAILURE);
    }

    struct planet* planet;
    planet = calloc(1, sizeof *planet);
    if (planet == NULL) goto memory_error;

    planet->vertices = calloc(PLANET_MAX_VERTICES, sizeof *planet->vertices);
    planet->normals  = calloc(PLANET_MAX_VERTICES, sizeof *planet->normals);
    planet->indices  = calloc(PLANET_MAX_INDICES, sizeof *planet->indices);
    planet->generator_vertices =
        calloc(PLANET_MAX_VERTICES, sizeof *planet->vertices);
    planet->generator_normals =
        calloc(PLANET_MAX_VERTICES, sizeof *planet->normals);
    planet->generator_indices =
        calloc(PLANET_MAX_INDICES, sizeof *planet->indices);

    if (!planet->vertices || !planet->normals || !planet->indices ||
        !planet->generator_vertices || !planet->generator_indices ||
        !planet->generator_normals)
        goto memory_error;

    planet->configured_params.subdivisions     = subdivisions;
    planet->configured_params.noise_gain       = NOISE_INITIAL_GAIN;
    planet->configured_params.noise_frequency  = NOISE_INITIAL_FREQUENCY;
    planet->configured_params.noise_lacunarity = NOISE_INITIAL_LACUNARITY;
    planet->configured_params.noise_layers     = NOISE_INITIAL_LAYERS;
    planet->configured_params.noise_scale      = NOISE_INITIAL_SCALE;

    planet->simplex = simplex_context_create(0);
    planet->mutex   = SDL_CreateMutex();
    if (!planet->mutex) goto memory_error;

    planet->thread = SDL_CreateThread(
        (SDL_ThreadFunction)planet_generation_main,
        "planet generator thread",
        planet
    );

    return planet;

memory_error:
    fprintf(stderr, "ERROR: failed to allocate planet\n");
    exit(EXIT_FAILURE);
}

void
planet_destroy(struct planet* planet)
{
    if (planet == NULL) return;
    simplex_context_destroy(planet->simplex);
    set_shutdown_signal(planet);
    SDL_WaitThread(planet->thread, NULL);
    SDL_DestroyMutex(planet->mutex);
    free(planet->vertices);
    free(planet->indices);
    free(planet->normals);
    free(planet->generator_vertices);
    free(planet->generator_indices);
    free(planet->generator_normals);
    free(planet);
}

uint64_t
planet_get_iteration(struct planet* planet)
{
    SDL_LockMutex(planet->mutex);
    uint64_t id = planet->id;
    SDL_UnlockMutex(planet->mutex);
    return id;
}

void
planet_set_subdivisions(struct planet* planet, uint32_t subdivisions)
{
    if (subdivisions == 0 || subdivisions > PLANET_MAX_SUBDIVISIONS) {
        fprintf(stderr, "ERROR: max planet subdivisions exceeded\n");
        exit(EXIT_FAILURE);
    }
    SDL_LockMutex(planet->mutex);
    planet->configured_params.subdivisions = subdivisions;
    SDL_UnlockMutex(planet->mutex);
}

void
planet_set_noise_layers(struct planet* planet, uint32_t layers)
{
    SDL_LockMutex(planet->mutex);
    planet->configured_params.noise_layers = layers;
    SDL_UnlockMutex(planet->mutex);
}

void
planet_set_noise_gain(struct planet* planet, float gain)
{
    SDL_LockMutex(planet->mutex);
    planet->configured_params.noise_gain = gain;
    SDL_UnlockMutex(planet->mutex);
}

void
planet_set_noise_frequency(struct planet* planet, float frequency)
{
    SDL_LockMutex(planet->mutex);
    planet->configured_params.noise_frequency = frequency;
    SDL_UnlockMutex(planet->mutex);
}

void
planet_set_noise_lacunarity(struct planet* planet, float lacunarity)
{
    SDL_LockMutex(planet->mutex);
    planet->configured_params.noise_lacunarity = lacunarity;
    SDL_UnlockMutex(planet->mutex);
}

void
planet_set_noise_scale(struct planet* planet, float scale)
{
    SDL_LockMutex(planet->mutex);
    planet->configured_params.noise_scale = scale;
    SDL_UnlockMutex(planet->mutex);
}

struct planet_mesh
planet_acquire_mesh(struct planet* planet)
{
    SDL_LockMutex(planet->mutex);
    return (struct planet_mesh){
        .iteration    = planet->id,
        .index_count  = planet->index_count,
        .vertex_count = planet->vertex_count,
        .vertices     = planet->vertices,
        .normals      = planet->normals,
        .indices      = planet->indices,
    };
}

void
planet_release_mesh(struct planet* planet)
{
    SDL_UnlockMutex(planet->mutex);
}
