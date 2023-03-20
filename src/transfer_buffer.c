#include "transfer_buffer.h"

void
transfer_buffer_destroy(struct vulkano* vk, struct transfer_buffer* transfer)
{
    if (transfer->buffer.handle == VK_NULL_HANDLE) return;
    vkDestroyFence(vk->device, transfer->fence, NULL);
    vkDestroySemaphore(vk->device, transfer->semaphore, NULL);
    vkUnmapMemory(vk->device, transfer->buffer.memory);
    vulkano_buffer_destroy(vk, &transfer->buffer);
    *transfer = (struct transfer_buffer){0};
}

struct transfer_buffer
transfer_buffer_create(
    struct vulkano* vk,
    size_t          capacity,
    VkCommandBuffer cmd,
    VulkanoError*   error
)
{
    if (*error) return (struct transfer_buffer){0};

    struct transfer_buffer transfer = {
        .capacity = capacity,
        .buffer   = vulkano_buffer_create(
            vk,
            (struct VkBufferCreateInfo){
                  .size  = capacity,
                  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            },
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            error
        ),
        .cmd   = cmd,
        .fence = vulkano_create_fence(vk, (struct VkFenceCreateInfo){0}, error),
        .semaphore = vulkano_create_semaphore(
            vk, (struct VkSemaphoreCreateInfo){0}, error
        ),
    };
    if (*error) {
        transfer_buffer_destroy(vk, &transfer);
        return transfer;
    }

    VULKANO_CHECK(
        vkMapMemory(
            vk->device,
            transfer.buffer.memory,
            0,
            capacity,
            0,
            (void**)&transfer.mapped_memory
        ),
        error
    );
    if (*error) {
        transfer_buffer_destroy(vk, &transfer);
        return transfer;
    }

    return transfer;
}

void
transfer_buffer_record_command_buffer(
    struct vulkano* vk, struct transfer_buffer* transfer, VulkanoError* error
)
{
    if (*error) return;

    VULKANO_CHECK(
        vkBeginCommandBuffer(
            transfer->cmd,
            (struct VkCommandBufferBeginInfo[]){
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                },
            }
        ),
        error
    );
    if (*error) return;

    if (transfer->head > transfer->capacity)
        transfer->head = transfer->capacity;
    size_t size = transfer->head;

    VULKANO_CHECK(
        vkFlushMappedMemoryRanges(
            vk->device,
            1,
            (struct VkMappedMemoryRange[]){
                {
                    .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .memory = transfer->buffer.memory,
                    .offset = 0,
                    .size   = size,
                },
            }
        ),
        error
    );
    if (*error) return;

    for (size_t i = 0; i < transfer->record_count; i++) {
        struct transfer_record* record = transfer->records + i;
        vkCmdCopyBuffer(
            transfer->cmd,
            transfer->buffer.handle,
            record->dst_handle,
            1,
            &record->buffer_copy
        );
    }

    transfer->record_count = 0;
    transfer->head         = 0;

    VULKANO_CHECK(vkEndCommandBuffer(transfer->cmd), error);
    if (*error) return;
}

void
transfer_buffer_flush_async(
    struct vulkano* vk, struct transfer_buffer* transfer, VulkanoError* error
)
{
    if (*error) return;

    transfer_buffer_record_command_buffer(vk, transfer, error);
    if (*error) return;

    VULKANO_CHECK(
        vkQueueSubmit(
            vk->gpu.graphics_queue,
            1,
            (const VkSubmitInfo[]){{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &transfer->cmd,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = &transfer->semaphore,
            }},
            VK_NULL_HANDLE
        ),
        error
    );
    if (*error) return;
}

void
transfer_buffer_flush_sync(
    struct vulkano* vk, struct transfer_buffer* transfer, VulkanoError* error
)
{
    if (*error) return;

    transfer_buffer_record_command_buffer(vk, transfer, error);
    if (*error) return;

    VULKANO_CHECK(
        vkQueueSubmit(
            vk->gpu.graphics_queue,
            1,
            (const VkSubmitInfo[]){{
                .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers    = &transfer->cmd,
            }},
            transfer->fence
        ),
        error
    );
    if (*error) return;

    VULKANO_CHECK(
        vkWaitForFences(
            vk->device, 1, &transfer->fence, VK_TRUE, VULKANO_TIMEOUT
        ),
        error
    );
    if (*error) return;

    VULKANO_CHECK(vkResetFences(vk->device, 1, &transfer->fence), error);
    if (*error) return;
}

void
transfer_buffer_copy(
    struct vulkano*         vk,
    struct transfer_buffer* transfer,
    struct vulkano_buffer   dst,
    size_t                  dst_offset,
    void*                   data,
    size_t                  datasize,
    VulkanoError*           error
)
{
    if (*error) return;

    if (datasize > transfer->capacity) {
        *error = VULKANO_ERROR_CODE_OUT_OF_MEMORY;
        fprintf(stderr, "ERROR: transfer exceeds transfer buffer capacity\n");
        return;
    }
    if (transfer->head >= transfer->capacity ||
        transfer->capacity - transfer->head < datasize ||
        transfer->record_count ==
            (sizeof transfer->records / sizeof *transfer->records)) {
        transfer_buffer_flush_sync(vk, transfer, error);
        if (*error) return;
    }

    const size_t alignment = vk->gpu.properties.limits.nonCoherentAtomSize;

    memcpy(transfer->mapped_memory + transfer->head, data, datasize);
    size_t padding = alignment - (datasize % alignment);

    transfer->records[transfer->record_count++] = (struct transfer_record){
        .dst_handle = dst.handle,
        .buffer_copy =
            (struct VkBufferCopy){
                .srcOffset = transfer->head,
                .dstOffset = dst_offset,
                .size      = datasize + padding,
            },
    };
    transfer->head += datasize + padding;
}
