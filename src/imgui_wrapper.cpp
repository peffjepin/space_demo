#include "imgui_wrapper.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl2.h"
#include "../imgui/imgui_impl_vulkan.h"

#include <stdarg.h>

VkDescriptorPool imgui_descriptor_pool;

static void
check_result(VkResult result)
{
    if (result == VK_SUCCESS) return;
    fprintf(stderr, "IMGUI VULKAN ERROR: VkResult = %d\n", result);
    exit(EXIT_FAILURE);
}

void
imgui_init(struct vulkano* vk, SDL_Window* window, VkCommandBuffer cmd)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes    = pool_sizes;
    VkResult result         = vkCreateDescriptorPool(
        vk->device, &pool_info, NULL, &imgui_descriptor_pool
    );
    check_result(result);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo imvk = {0};
    imvk.Instance                  = vk->instance;
    imvk.PhysicalDevice            = vk->gpu.handle;
    imvk.Device                    = vk->device;
    imvk.QueueFamily               = vk->gpu.graphics_queue_family;
    imvk.Queue                     = vk->gpu.graphics_queue;
    imvk.DescriptorPool            = imgui_descriptor_pool;
    imvk.MinImageCount             = vk->swapchain.image_count;
    imvk.ImageCount                = vk->swapchain.image_count;
    imvk.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    imvk.CheckVkResultFn           = check_result;

    ImGui_ImplVulkan_Init(&imvk, vk->swapchain.render_pass);
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
}

void
imgui_teardown(struct vulkano* vk)
{
    vkDeviceWaitIdle(vk->device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(vk->device, imgui_descriptor_pool, NULL);
}

bool
imgui_process_event(const SDL_Event* event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse && (event->type == SDL_MOUSEBUTTONDOWN ||
                                event->type == SDL_MOUSEBUTTONUP)) {
        return true;
    }
    if (io.WantCaptureKeyboard &&
        (event->type == SDL_KEYUP || event->type == SDL_KEYDOWN ||
         event->type == SDL_TEXTINPUT)) {
        return true;
    }
    return false;
}

void
imgui_start_frame(void)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void
imgui_finish_frame(VkCommandBuffer cmd)
{
    ImGui::EndFrame();
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
}

void
imgui_begin(const char* name, enum imgui_wrapper_window_flags wrapper_flags)
{
    ImGuiWindowFlags flags = 0;
    if (wrapper_flags & IMGUI_WINDOW_ALWAYS_AUTO_RESIZE)
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if (wrapper_flags & IMGUI_WINDOW_NO_RESIZE)
        flags |= ImGuiWindowFlags_NoResize;
    ImGui::Begin(name, NULL, flags);
}

void
imgui_end(void)
{
    ImGui::End();
}

void
imgui_text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

void
imgui_set_next_window_size(float width, float height)
{
    ImGui::SetNextWindowSize(ImVec2(width, height));
}

void
imgui_set_next_window_size_constraints(
    float min_width, float min_height, float max_width, float max_height
)
{
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(min_width, min_height), ImVec2(max_width, max_height)
    );
}

void
imgui_set_next_window_position(float left, float top)
{
    ImGui::SetNextWindowPos(ImVec2(left, top));
}

void
imgui_set_next_window_position_pivot(
    float left, float top, float pivot_x, float pivot_y
)
{
    ImGui::SetNextWindowPos(
        ImVec2(left, top), ImGuiCond_Always, ImVec2(pivot_x, pivot_y)
    );
}

void
imgui_sliderf(const char* name, float* value, float min, float max)
{
    ImGui::SliderFloat(name, value, min, max);
}

void
imgui_slideri(const char* name, int* value, int min, int max)
{
    ImGui::SliderInt(name, value, min, max);
}
