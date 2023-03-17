#define VULKANO_IMPLEMENTATION
#define VULKANO_ENABLE_DEFAULT_VALIDATION_LAYERS
#define VULKANO_ENABLE_DEFAULT_GRAPHICS_EXTENSIONS
#define VULKANO_INTEGRATE_SDL

#include "imgui_wrapper.h"
#include "vulkano.h"

int
main(void)
{
    VulkanoError error = 0;
    struct vulkano_sdl vksdl = vulkano_sdl_create(
        (struct vulkano_config){0},
        (struct sdl_config){.window_flags = SDL_WINDOW_RESIZABLE},
        &error
    );
    if (error) exit(EXIT_FAILURE);

    VkRenderPass render_pass = vulkano_create_render_pass(
        &vksdl.vk,
        (struct VkRenderPassCreateInfo){
            .attachmentCount = 2,
            .pAttachments =
                (struct VkAttachmentDescription[]){
                    {
                        .format = 0,  // match swapchain
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    },
                    {
                        .format = VULKANO_DEPTH_FORMAT,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    },
                },
            .subpassCount = 1,
            .pSubpasses =
                (struct VkSubpassDescription[]){
                    {
                        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                        .colorAttachmentCount = 1,
                        .pColorAttachments =
                            (struct VkAttachmentReference[]){
                                {
                                    .attachment = 0,
                                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                },
                            },
                        .pDepthStencilAttachment =
                            (struct VkAttachmentReference[]){
                                {
                                    .attachment = 1,
                                    .layout =
                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                },
                            },
                    },
                },
        },
        &error
    );
    vulkano_configure_swapchain(&vksdl.vk, render_pass, 2, &error);
    VkCommandBuffer init_cmd =
        vulkano_acquire_single_use_command_buffer(&vksdl.vk, &error);
    if (error) {
        vulkano_sdl_destroy(&vksdl);
        exit(EXIT_FAILURE);
    }

    imgui_init(&vksdl.vk, vksdl.sdl, init_cmd);

    vulkano_submit_single_use_command_buffer(&vksdl.vk, init_cmd, &error);
    if (error) goto teardown;

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
                default:
                    break;
            }
        }

        struct vulkano_frame vkframe = {
            .clear = {0.0, 0.0, 0.0, 1.0},
        };
        vulkano_frame_acquire(&vksdl.vk, &vkframe, &error);
        if (error) goto teardown;

        VkCommandBuffer cmd = vkframe.state.render_command;
        imgui_begin_frame();
        imgui_end_frame(cmd);

        vulkano_frame_submit(&vksdl.vk, &vkframe, (struct VkSubmitInfo){0}, &error);
        if (error) goto teardown;
    }

teardown:
    vkDeviceWaitIdle(vksdl.vk.device);
    imgui_teardown(&vksdl.vk);
    vkDestroyRenderPass(vksdl.vk.device, render_pass, NULL);
    vulkano_sdl_destroy(&vksdl);
    return error;
}
