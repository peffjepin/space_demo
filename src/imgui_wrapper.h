#ifndef IMGUI_WRAPPER_H
#define IMGUI_WRAPPER_H

#include <SDL2/SDL.h>

#include "vulkano.h"

enum imgui_wrapper_window_flags {
    IMGUI_WINDOW_NO_RESIZE          = 1 << 0,
    IMGUI_WINDOW_ALWAYS_AUTO_RESIZE = 1 << 1,
};


#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

void imgui_init(struct vulkano* vk, SDL_Window*, VkCommandBuffer);
void imgui_teardown(struct vulkano* vk);
bool imgui_process_event(const SDL_Event* event);
void imgui_start_frame(void);
void imgui_finish_frame(VkCommandBuffer cmd);
void imgui_set_next_window_size(float width, float height);
void imgui_set_next_window_size_constraints(float min_width, float min_height, float max_width, float max_height);
void imgui_set_next_window_position(float left, float top);
void imgui_set_next_window_position_pivot(float left, float top, float pivot_x, float pivot_y);
void imgui_begin(const char* name, enum imgui_wrapper_window_flags);
void imgui_text(const char* fmt, ...);
void imgui_sliderf(const char* name, float* value, float min, float max);
void imgui_slideri(const char* name, int* value, int min, int max);
void imgui_end(void);

// clang-format on

#ifdef __cplusplus
}
#endif

#endif  // IMGUI_WRAPPER_H
