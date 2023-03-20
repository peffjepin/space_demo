#include "renderer.h"

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "3d.h"
#include "planet.h"
#include "transfer_buffer.h"
#include "imgui_wrapper.h"

#define CONCURRENT_FRAMES 2

#define TRANSFER_BUFFER_SIZE                                                   \
    (PLANET_MAX_VERTICES * sizeof(struct vec3) * 2 + 2 * sizeof(struct mat4) + \
     sizeof(uint32_t) * PLANET_MAX_INDICES + 10000)

struct demo_renderer {
    struct vulkano* vk;

    struct vec3 camera_position;
    struct vec3 camera_direction;

    struct {
        struct mat4 view;
        struct mat4 proj;
    } ubo;

    size_t                ubo_size_per_frame;
    struct vulkano_buffer uniform_buffer;

    size_t                vertices_buffer_size_per_frame;
    struct vulkano_buffer vertices_buffer;

    size_t                indices_buffer_size_per_frame;
    struct vulkano_buffer indices_buffer;

    size_t                normals_buffer_size_per_frame;
    struct vulkano_buffer normals_buffer;

    VkRenderPass          render_pass;
    VkShaderModule        vertex_shader;
    VkShaderModule        fragment_shader;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool      descriptor_pool;
    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;

    VkDescriptorSet        descriptor_sets[CONCURRENT_FRAMES];
    VkCommandPool          transfer_command_pool;
    struct transfer_buffer transfer_buffers[CONCURRENT_FRAMES];

    struct {
        struct planet* planet;
        uint64_t       id;
    } buffered_planets[CONCURRENT_FRAMES];
};

void
renderer_set_camera_position(
    struct demo_renderer* renderer, float x, float y, float z
)
{
    renderer->camera_position.x = x;
    renderer->camera_position.y = y;
    renderer->camera_position.z = z;

    renderer->ubo.view = view_matrix(
        renderer->camera_position,
        renderer->camera_direction,
        (struct vec3){0.0f, -1.0f, 0.0f}
    );
}

void
renderer_set_camera_direction(
    struct demo_renderer* renderer, float x, float y, float z
)
{
    renderer->camera_direction.x = x;
    renderer->camera_direction.y = y;
    renderer->camera_direction.z = z;

    renderer->ubo.view = view_matrix(
        renderer->camera_position,
        renderer->camera_direction,
        (struct vec3){0.0f, -1.0f, 0.0f}
    );
}

void
renderer_set_camera_target(
    struct demo_renderer* renderer, float x, float y, float z
)
{
    renderer->camera_direction.x = x - renderer->camera_position.x;
    renderer->camera_direction.y = y - renderer->camera_position.y;
    renderer->camera_direction.z = z - renderer->camera_position.z;

    renderer->ubo.view = view_matrix(
        renderer->camera_position,
        renderer->camera_direction,
        (struct vec3){0.0f, -1.0f, 0.0f}
    );
}

struct vulkano_data read_file_content(const char* filepath);

#define ALIGN(size, alignment)                                                 \
    ((((size) + (alignment)-1) / (alignment)) * (alignment))

struct demo_renderer*
renderer_create(struct vulkano* vk)
{
    struct demo_renderer* renderer;
    renderer = calloc(1, sizeof *renderer);
    if (renderer == NULL) {
        fprintf(stderr, "ERROR: failed to allocate renderer\n");
        exit(EXIT_FAILURE);
    }
    renderer->vk               = vk;
    renderer->camera_position  = (struct vec3){0.0f, 0.0f, 0.0f};
    renderer->camera_direction = (struct vec3){0.0f, 0.0f, 1.0f};
    renderer->ubo.view         = view_matrix(
        renderer->camera_position,
        renderer->camera_direction,
        (struct vec3){0.0f, -1.0f, 1.0f}
    );
    renderer->ubo.proj = projection_matrix(
        60.0f,
        (float)renderer->vk->swapchain.extent.width /
            (float)renderer->vk->swapchain.extent.height,
        0.1f,
        100.0f
    );

    VulkanoError error = 0;

    // create render pass and configure swapchain
    renderer->render_pass = vulkano_create_render_pass(
        vk,
        (struct VkRenderPassCreateInfo){
            .attachmentCount = 2,
            .pAttachments =
                (struct VkAttachmentDescription[]){
                    {
                        .format         = 0,  // match swapchain
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    },
                    {
                        .format         = VULKANO_DEPTH_FORMAT,
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .finalLayout =
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    },
                },
            .subpassCount = 1,
            .pSubpasses =
                (struct VkSubpassDescription[]){
                    {
                        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
                        .colorAttachmentCount = 1,
                        .pColorAttachments =
                            (struct VkAttachmentReference[]){
                                {
                                    .attachment = 0,
                                    .layout =
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
    vulkano_configure_swapchain(
        vk, renderer->render_pass, CONCURRENT_FRAMES, &error
    );

    // create buffers
    //
    const size_t per_frame_alignment =
        vk->gpu.properties.limits.minUniformBufferOffsetAlignment;

    renderer->vertices_buffer_size_per_frame =
        ALIGN(PLANET_MAX_VERTICES * sizeof(struct vec3), per_frame_alignment);
    renderer->vertices_buffer = vulkano_buffer_create(
        vk,
        (struct VkBufferCreateInfo){
            .size =
                renderer->vertices_buffer_size_per_frame * CONCURRENT_FRAMES,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &error
    );

    renderer->normals_buffer_size_per_frame =
        ALIGN(PLANET_MAX_VERTICES * sizeof(struct vec3), per_frame_alignment);
    renderer->normals_buffer = vulkano_buffer_create(
        vk,
        (struct VkBufferCreateInfo){
            .size = renderer->normals_buffer_size_per_frame * CONCURRENT_FRAMES,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &error
    );

    renderer->indices_buffer_size_per_frame =
        ALIGN(PLANET_MAX_INDICES * sizeof(uint32_t), per_frame_alignment);
    renderer->indices_buffer = vulkano_buffer_create(
        vk,
        (struct VkBufferCreateInfo){
            .size = renderer->indices_buffer_size_per_frame * CONCURRENT_FRAMES,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &error
    );

    renderer->ubo_size_per_frame =
        ALIGN(sizeof(renderer->ubo), per_frame_alignment);
    renderer->uniform_buffer = vulkano_buffer_create(
        vk,
        (struct VkBufferCreateInfo){
            .size  = renderer->ubo_size_per_frame * CONCURRENT_FRAMES,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &error
    );

    // create pipeline components and pipeline
    //
    struct vulkano_data vertex_shader_content =
        read_file_content("build/planet.vert.spv");
    struct vulkano_data fragment_shader_content =
        read_file_content("build/planet.frag.spv");

    renderer->vertex_shader =
        vulkano_create_shader_module(vk, vertex_shader_content, &error);
    renderer->fragment_shader =
        vulkano_create_shader_module(vk, fragment_shader_content, &error);
    free(vertex_shader_content.data);
    free(fragment_shader_content.data);

    renderer->descriptor_set_layout = vulkano_create_descriptor_set_layout(
        vk,
        (struct VkDescriptorSetLayoutCreateInfo){
            .bindingCount = 1,
            .pBindings =
                (struct VkDescriptorSetLayoutBinding[]){
                    {
                        .binding         = 0,
                        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = 1,
                        .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT,
                    },
                },
        },
        &error
    );
    renderer->descriptor_pool = vulkano_create_descriptor_pool(
        vk,
        (struct VkDescriptorPoolCreateInfo){
            .maxSets       = CONCURRENT_FRAMES,
            .poolSizeCount = 1,
            .pPoolSizes =
                (struct VkDescriptorPoolSize[]){
                    {
                        .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = CONCURRENT_FRAMES,
                    },
                },
        },
        &error
    );
    renderer->pipeline_layout = vulkano_create_pipeline_layout(
        vk,
        (struct VkPipelineLayoutCreateInfo){
            .setLayoutCount = 1,
            .pSetLayouts    = &renderer->descriptor_set_layout,
        },
        &error
    );
    renderer->pipeline = vulkano_create_graphics_pipeline(
        vk,
        (struct vulkano_pipeline_config){
            .stage_count = 2,
            .stages =
                {
                    {
                        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = renderer->vertex_shader,
                        .pName  = "main",
                    },
                    {
                        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = renderer->fragment_shader,
                        .pName  = "main",
                    },
                },
            .vertex_input_state =
                {
                    .vertexBindingDescriptionCount = 2,
                    .pVertexBindingDescriptions =
                        (struct VkVertexInputBindingDescription[]){
                            {
                                .binding   = 0,
                                .stride    = sizeof(struct vec3),
                                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                            },
                            {
                                .binding   = 1,
                                .stride    = sizeof(struct vec3),
                                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                            },
                        },
                    .vertexAttributeDescriptionCount = 2,
                    .pVertexAttributeDescriptions =
                        (struct VkVertexInputAttributeDescription[]){
                            {
                                .binding  = 0,
                                .location = 0,
                                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                                .offset   = 0,
                            },
                            {
                                .binding  = 1,
                                .location = 1,
                                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                                .offset   = 0,
                            },
                        },
                },
            .input_assembly_state =
                {
                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                },
            .viewport_state =
                {
                    .viewportCount = 1,
                    .pViewports =
                        (struct VkViewport[]){
                            VULKANO_VIEWPORT(vk),
                        },
                    .scissorCount = 1,
                    .pScissors =
                        (struct VkRect2D[]){
                            VULKANO_SCISSOR(vk),
                        },
                },
            .rasterization_state =
                {
                    .polygonMode = VK_POLYGON_MODE_LINE,
                    .cullMode    = VK_CULL_MODE_NONE,
                    .frontFace   = VK_FRONT_FACE_CLOCKWISE,
                    .lineWidth   = 1,
                },
            .depth_stencil_state =
                {
                    .depthTestEnable  = VK_FALSE,
                    .depthWriteEnable = VK_FALSE,
                    .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL,
                },
            .color_blend_state =
                {

                    .attachmentCount = 1,
                    .pAttachments =
                        (struct VkPipelineColorBlendAttachmentState[]){
                            {
                                .blendEnable = VK_TRUE,
                                .srcColorBlendFactor =
                                    VK_BLEND_FACTOR_SRC_ALPHA,
                                .dstColorBlendFactor =
                                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                .colorBlendOp        = VK_BLEND_OP_ADD,
                                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                .alphaBlendOp        = VK_BLEND_OP_ADD,
                                .colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT |
                                                  VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_R_BIT,
                            },
                        },
                },
            .dynamic_state =
                {
                    .dynamicStateCount = 2,
                    .pDynamicStates =
                        (VkDynamicState[]){
                            VK_DYNAMIC_STATE_SCISSOR,
                            VK_DYNAMIC_STATE_VIEWPORT,
                        },
                },
            .layout      = renderer->pipeline_layout,
            .render_pass = renderer->render_pass,
        },
        &error
    );

    // create/write descriptor sets
    //
    VkDescriptorSetLayout set_layouts[CONCURRENT_FRAMES];
    for (size_t i = 0; i < CONCURRENT_FRAMES; i++) {
        set_layouts[i] = renderer->descriptor_set_layout;
    }
    vulkano_allocate_descriptor_sets(
        vk,
        (VkDescriptorSetAllocateInfo){
            .descriptorPool     = renderer->descriptor_pool,
            .descriptorSetCount = CONCURRENT_FRAMES,
            .pSetLayouts        = set_layouts,
        },
        renderer->descriptor_sets,
        &error
    );
    VkDescriptorBufferInfo ubo_info = {
        .buffer = renderer->uniform_buffer.handle,
        .offset = 0,
        .range  = renderer->ubo_size_per_frame,
    };
    for (size_t i = 0; i < CONCURRENT_FRAMES; i++) {
        VkWriteDescriptorSet writes[] = {
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = renderer->descriptor_sets[i],
                .dstBinding      = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo     = &ubo_info,
            },
        };
        vkUpdateDescriptorSets(
            vk->device, sizeof writes / sizeof *writes, writes, 0, NULL
        );
        ubo_info.offset += ubo_info.range;
    }

    // create transfer buffers
    //
    VkCommandBuffer transfer_command_buffers[CONCURRENT_FRAMES];
    renderer->transfer_command_pool = vulkano_create_command_pool(
        vk,
        (VkCommandPoolCreateInfo){
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vk->gpu.graphics_queue_family,
        },
        &error
    );
    vulkano_allocate_command_buffers(
        vk,
        (struct VkCommandBufferAllocateInfo){
            .commandPool        = renderer->transfer_command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = CONCURRENT_FRAMES,
        },
        transfer_command_buffers,
        &error
    );
    for (size_t i = 0; i < CONCURRENT_FRAMES; i++) {
        renderer->transfer_buffers[i] = transfer_buffer_create(
            vk, TRANSFER_BUFFER_SIZE, transfer_command_buffers[i], &error
        );
    }
    if (error) exit(EXIT_FAILURE);

    return renderer;
}

void
renderer_destroy(struct demo_renderer* renderer)
{
    vkDeviceWaitIdle(renderer->vk->device);

    for (size_t i = 0; i < CONCURRENT_FRAMES; i++) {
        transfer_buffer_destroy(renderer->vk, &renderer->transfer_buffers[i]);
    }
    vkDestroyCommandPool(
        renderer->vk->device, renderer->transfer_command_pool, NULL
    );

    vulkano_buffer_destroy(renderer->vk, &renderer->uniform_buffer);
    vulkano_buffer_destroy(renderer->vk, &renderer->indices_buffer);
    vulkano_buffer_destroy(renderer->vk, &renderer->vertices_buffer);
    vulkano_buffer_destroy(renderer->vk, &renderer->normals_buffer);
    vkDestroyRenderPass(renderer->vk->device, renderer->render_pass, NULL);
    vkDestroyShaderModule(renderer->vk->device, renderer->vertex_shader, NULL);
    vkDestroyShaderModule(
        renderer->vk->device, renderer->fragment_shader, NULL
    );
    vkDestroyDescriptorSetLayout(
        renderer->vk->device, renderer->descriptor_set_layout, NULL
    );
    vkDestroyDescriptorPool(
        renderer->vk->device, renderer->descriptor_pool, NULL
    );
    vkDestroyPipelineLayout(
        renderer->vk->device, renderer->pipeline_layout, NULL
    );
    vkDestroyPipeline(renderer->vk->device, renderer->pipeline, NULL);
}

VkSubmitInfo
renderer_draw(
    struct demo_renderer* renderer,
    VkCommandBuffer       cmd,
    size_t                frame_index,
    struct planet*        planet
)
{
    VulkanoError error = 0;

    renderer->ubo.proj = projection_matrix(
        60.0f,
        (float)renderer->vk->swapchain.extent.width /
            (float)renderer->vk->swapchain.extent.height,
        0.1f,
        100.0f
    );

    struct transfer_buffer* transfer = renderer->transfer_buffers + frame_index;
    transfer_buffer_copy(
        renderer->vk,
        transfer,
        renderer->uniform_buffer,
        renderer->ubo_size_per_frame * frame_index,
        &renderer->ubo,
        (sizeof renderer->ubo),
        &error
    );
    if (renderer->buffered_planets[frame_index].planet != planet ||
        renderer->buffered_planets[frame_index].id != planet->id) {
        transfer_buffer_copy(
            renderer->vk,
            transfer,
            renderer->vertices_buffer,
            renderer->vertices_buffer_size_per_frame * frame_index,
            planet->vertices,
            (sizeof *planet->vertices) * planet->vertex_count,
            &error
        );
        transfer_buffer_copy(
            renderer->vk,
            transfer,
            renderer->normals_buffer,
            renderer->normals_buffer_size_per_frame * frame_index,
            planet->normals,
            (sizeof *planet->normals) * planet->vertex_count,
            &error
        );
        transfer_buffer_copy(
            renderer->vk,
            transfer,
            renderer->indices_buffer,
            renderer->indices_buffer_size_per_frame * frame_index,
            planet->indices,
            (sizeof *planet->indices) * planet->index_count,
            &error
        );
        renderer->buffered_planets[frame_index].planet = planet;
        renderer->buffered_planets[frame_index].id     = planet->id;
    }
    transfer_buffer_flush_async(renderer->vk, transfer, &error);
    if (error) exit(EXIT_FAILURE);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderer->pipeline_layout,
        0,
        1,
        (VkDescriptorSet[]){renderer->descriptor_sets[frame_index]},
        0,
        NULL
    );

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
    vkCmdBindIndexBuffer(
        cmd,
        renderer->indices_buffer.handle,
        renderer->indices_buffer_size_per_frame * frame_index,
        VK_INDEX_TYPE_UINT32
    );
    vkCmdBindVertexBuffers(
        cmd,
        0,
        2,
        (VkBuffer[]){
            renderer->vertices_buffer.handle,
            renderer->normals_buffer.handle,
        },
        (VkDeviceSize[]){
            renderer->vertices_buffer_size_per_frame * frame_index,
            renderer->normals_buffer_size_per_frame * frame_index,
        }
    );
    VkViewport viewport = VULKANO_VIEWPORT(renderer->vk);
    VkRect2D   scissor  = VULKANO_SCISSOR(renderer->vk);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdDrawIndexed(cmd, planet->index_count, 1, 0, 0, 0);

    static const VkPipelineStageFlags STAGE_MASK =
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    return (VkSubmitInfo){
        .waitSemaphoreCount = 1,
        .pWaitDstStageMask  = &STAGE_MASK,
        .pWaitSemaphores    = &transfer->semaphore,
    };
}

struct vulkano_data
read_file_content(const char* filepath)
{
    struct vulkano_data data = {0};

    FILE* fp = fopen(filepath, "rb");
    int   end;

    if (!fp) {
        fprintf(
            stderr,
            "ERROR: failed to open file `%s` (%s)\n",
            filepath,
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }
    if (fseek(fp, 0, SEEK_END)) {
        fprintf(
            stderr,
            "ERROR: failed to file operation `%s` (%s)\n",
            filepath,
            strerror(errno)
        );
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    if ((end = ftell(fp)) < 0) {
        fprintf(
            stderr,
            "ERROR: failed to file operation `%s` (%s)\n",
            filepath,
            strerror(errno)
        );
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    data.size = end;
    if (fseek(fp, 0, SEEK_SET)) {
        fprintf(
            stderr,
            "ERROR: failed to file operation `%s` (%s)\n",
            filepath,
            strerror(errno)
        );
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    data.data = (uint8_t*)malloc(data.size);
    if (data.data == NULL) {
        fprintf(stderr, "ERROR: out of memory\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    if (fread(data.data, 1, data.size, fp) < data.size) {
        fprintf(
            stderr,
            "ERROR: failed to read file contents `%s` (%s)\n",
            filepath,
            strerror(errno)
        );
        fclose(fp);
        exit(EXIT_FAILURE);
    };

    fclose(fp);
    return data;
}
