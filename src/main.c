#define VULKANO_IMPLEMENTATION
#define VULKANO_ENABLE_DEFAULT_VALIDATION_LAYERS
#define VULKANO_ENABLE_DEFAULT_GRAPHICS_EXTENSIONS
#define VULKANO_INTEGRATE_SDL
#include "vulkano.h"

#include "renderer.h"
#include "planet.h"
#include "imgui_wrapper.h"

int
main(void)
{
    VulkanoError       error = 0;
    struct vulkano_sdl vksdl = vulkano_sdl_create(
        (struct vulkano_config){0},
        (struct sdl_config){
            // .window_flags = SDL_WINDOW_RESIZABLE,
            .left   = 100,
            .top    = 100,
            .width  = 1920,
            .height = 1080,
        },
        &error
    );
    if (error) exit(EXIT_FAILURE);

    Renderer renderer = renderer_create(&vksdl.vk);

    VkCommandBuffer init_cmd =
        vulkano_acquire_single_use_command_buffer(&vksdl.vk, &error);
    if (error) exit(EXIT_FAILURE);
    imgui_init(&vksdl.vk, vksdl.sdl, init_cmd);

    vulkano_submit_single_use_command_buffer(&vksdl.vk, init_cmd, &error);
    if (error) exit(EXIT_FAILURE);

    renderer_set_camera_position(renderer, 0.0f, 0.0f, -PLANET_RADIUS * 2);
    renderer_set_camera_target(renderer, 0.0f, 0.0f, 0.0f);

    static const uint32_t INITIAL_SUBDIVISIONS = PLANET_MAX_SUBDIVISIONS / 3;
    struct planet*        planet = planet_create(INITIAL_SUBDIVISIONS);

    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto teardown;
            }
            if (imgui_process_event(&event)) continue;

            switch (event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym == SDLK_q ||
                        event.key.keysym.sym == SDLK_ESCAPE)
                        goto teardown;
                    break;
                }
                default: break;
            }
        }

        struct vulkano_frame vkframe = {
            .clear = {0.0, 0.0, 0.0, 1.0},
        };
        vulkano_frame_acquire(&vksdl.vk, &vkframe, &error);
        if (error == VULKANO_ERROR_CODE_MINIMIZED) continue;
        if (error) goto teardown;

        VkCommandBuffer cmd = vkframe.state.render_command;
        VkSubmitInfo    submit_info =
            renderer_draw(renderer, cmd, vkframe.index, planet);

        imgui_start_frame();

        imgui_set_next_window_position_pivot(
            (float)vksdl.vk.swapchain.extent.width, 0.0f, 1.0f, 0.0f
        );
        imgui_set_next_window_size_constraints(
            0.0f,
            (float)vksdl.vk.swapchain.extent.height,
            10000.f,
            (float)vksdl.vk.swapchain.extent.height
        );
        imgui_begin(
            "control panel",
            IMGUI_WINDOW_ALWAYS_AUTO_RESIZE | IMGUI_WINDOW_NO_RESIZE
        );

        struct planet_mesh mesh = planet_acquire_mesh(planet);
        planet_release_mesh(planet);
        imgui_text("vertex_count: %d", mesh.vertex_count);

        static int previous_subdivisions = INITIAL_SUBDIVISIONS;
        static int subdivisions          = INITIAL_SUBDIVISIONS;
        imgui_slideri(
            "xubdivisions", &subdivisions, 1, PLANET_MAX_SUBDIVISIONS
        );
        if (subdivisions != previous_subdivisions) {
            previous_subdivisions = subdivisions;
            planet_set_subdivisions(planet, subdivisions);
        }

        static int previous_layers = NOISE_INITIAL_LAYERS;
        static int layers          = NOISE_INITIAL_LAYERS;
        imgui_slideri("layers", &layers, NOISE_MIN_LAYERS, NOISE_MAX_LAYERS);
        if (layers != previous_layers) {
            previous_layers = layers;
            planet_set_noise_layers(planet, layers);
        }

        static float previous_gain = NOISE_INITIAL_GAIN;
        static float gain          = NOISE_INITIAL_GAIN;
        imgui_sliderf("gain", &gain, NOISE_MIN_GAIN, NOISE_MAX_GAIN);
        if (gain != previous_gain) {
            previous_gain = gain;
            planet_set_noise_gain(planet, gain);
        }

        static float previous_frequency = NOISE_INITIAL_FREQUENCY;
        static float frequency          = NOISE_INITIAL_FREQUENCY;
        imgui_sliderf(
            "frequency", &frequency, NOISE_MIN_FREQUENCY, NOISE_MAX_FREQUENCY
        );
        if (frequency != previous_frequency) {
            previous_frequency = frequency;
            planet_set_noise_frequency(planet, frequency);
        }

        static float previous_lacunarity = NOISE_INITIAL_LACUNARITY;
        static float lacunarity          = NOISE_INITIAL_LACUNARITY;
        imgui_sliderf(
            "lacunarity",
            &lacunarity,
            NOISE_MIN_LACUNARITY,
            NOISE_MAX_LACUNARITY
        );
        if (lacunarity != previous_lacunarity) {
            previous_lacunarity = lacunarity;
            planet_set_noise_lacunarity(planet, lacunarity);
        }

        imgui_end();

        imgui_finish_frame(cmd);

        vulkano_frame_submit(&vksdl.vk, &vkframe, submit_info, &error);
        if (error) goto teardown;
    }

teardown:
    imgui_teardown(&vksdl.vk);
    renderer_destroy(renderer);
    vulkano_sdl_destroy(&vksdl);
    planet_destroy(planet);
    return error;
}
