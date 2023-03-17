#ifndef IMGUI_WRAPPER_H
#define IMGUI_WRAPPER_H

#include <SDL2/SDL.h>

#include "vulkano.h"


#ifdef __cplusplus
extern "C" {
#endif

void imgui_init(struct vulkano* vk, SDL_Window*, VkCommandBuffer);
void imgui_teardown(struct vulkano* vk);
bool imgui_process_event(const SDL_Event* event);
void imgui_begin_frame(void);
void imgui_end_frame(VkCommandBuffer cmd);

#ifdef __cplusplus
}
#endif

#endif  // IMGUI_WRAPPER_H
