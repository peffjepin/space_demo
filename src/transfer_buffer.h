#ifndef TRANSFER_BUFFER_H
#define TRANSFER_BUFFER_H

#include "vulkano.h"
#include <vulkan/vulkan.h>

#define TRANSFER_BUFFER_RECORD_CAPACITY 256

// TODO: should at some point be optimized to store an array of copies per
// dst_handle but this works fine for now

struct transfer_record {
    VkBuffer     dst_handle;
    VkBufferCopy buffer_copy;
};

struct transfer_buffer {
    struct vulkano_buffer  buffer;
    size_t                 capacity;
    size_t                 head;
    uint8_t*               mapped_memory;
    size_t                 record_count;
    struct transfer_record records[TRANSFER_BUFFER_RECORD_CAPACITY];

    VkCommandBuffer cmd;
    VkFence         fence;
    VkSemaphore     semaphore;
};

void transfer_buffer_destroy(struct vulkano*, struct transfer_buffer*);

struct transfer_buffer
transfer_buffer_create(struct vulkano*, size_t capacity, VkCommandBuffer, VulkanoError*);

void
transfer_buffer_record_command_buffer(struct vulkano*, struct transfer_buffer*, VulkanoError*);

void
transfer_buffer_flush_async(struct vulkano*, struct transfer_buffer*, VulkanoError*);

void
transfer_buffer_flush_sync(struct vulkano*, struct transfer_buffer*, VulkanoError*);

void
transfer_buffer_copy(struct vulkano*, struct transfer_buffer*, struct vulkano_buffer, size_t offset, void* data, size_t nbytes, VulkanoError*);

#endif  // TRANSFER_BUFFER_H
