#ifndef RENDERER_H
#define RENDERER_H

#include "vulkano.h"
#include "planet.h"

typedef struct demo_renderer* Renderer;

Renderer     renderer_create(struct vulkano* vk);
void         renderer_destroy(Renderer);
void         renderer_set_camera_position(Renderer, float x, float y, float z);
void         renderer_set_camera_direction(Renderer, float x, float y, float z);
void         renderer_set_camera_target(Renderer, float x, float y, float z);
VkSubmitInfo renderer_draw(
    Renderer, VkCommandBuffer cmd, size_t frame_index, Planet
);


#endif  // RENDERER_H
