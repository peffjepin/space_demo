#ifndef VULKANO_H
#define VULKANO_H

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

// available preprocessor define flags:
//
// VULKANO_DISABLE_VALIDATION_LAYERS
// VULKANO_ENABLE_DEFAULT_VALIDATION_LAYERS
// VULKANO_ENABLE_DEFAULT_GRAPHICS_EXTENSIONS
// VULKANO_INTEGRATE_SDL
// VULKANO_DISABLE_LOGGING

#ifndef VULKANO_LOG_FILE
#define VULKANO_LOG_FILE stdout
#endif

#ifndef VULKANO_TIMEOUT
#define VULKANO_TIMEOUT (5lu * 1000000000lu)  // 5 seconds
#endif

#ifndef VULKANO_DEPTH_FORMAT
#define VULKANO_DEPTH_FORMAT VK_FORMAT_D24_UNORM_S8_UINT
#endif

typedef int (*gpu_compare_function)(VkPhysicalDevice*, VkPhysicalDevice*);
typedef int (*surface_format_compare_function
)(VkSurfaceFormatKHR*, VkSurfaceFormatKHR*);
typedef int (*present_mode_compare_function
)(VkPresentModeKHR*, VkPresentModeKHR*);

// returns NULL on success, or an error message on an error
typedef const char* (*surface_creation_function)(VkInstance, VkSurfaceKHR*);
// tells the library how to query for window size
typedef void (*query_size_function)(uint32_t* width, uint32_t* height);

struct vulkano_config {
    // required functions
    surface_creation_function surface_creation;
    query_size_function       query_window_size;

    // All compare functions have default implementations
    gpu_compare_function            gpu_compare;
    surface_format_compare_function format_compare;
    present_mode_compare_function   present_compare;

    uint32_t     validation_layers_count;
    const char** validation_layers;
    uint32_t     instance_extensions_count;
    const char** instance_extensions;
    uint32_t     gpu_extensions_count;
    const char** gpu_extensions;
};

struct vulkano_data {
    uint32_t size;
    uint8_t* data;
};

struct vulkano_image_data {
    uint32_t size;
    uint8_t* pixels;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    VkFormat format;
};

struct vulkano_buffer {
    VkBuffer              handle;
    VkDeviceMemory        memory;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags memory_flags;
    uint64_t              capacity;
};

struct vulkano_image {
    VkImage               handle;
    VkDeviceMemory        memory;
    VkMemoryPropertyFlags memory_flags;
    VkImageLayout         layout;
};

struct vulkano_sampler {
    struct vulkano_image image;
    VkImageView          view;
    VkSampler            handle;
};

// TODO: this seems like it belongs with the swapchain, but has a separate
// lifetime because it's not recreated when the swapchain resources are
// recreated on resize. It works as is for now, but may be worth redesigning
// slightly in the future.
struct vulkano_per_frame_state {
    VkSemaphore     image_ready_for_use;
    VkSemaphore     rendering_commands_complete;
    VkFence         presentation_complete;
    VkCommandPool   command_pool;
    VkCommandBuffer render_command;
};

struct vulkano_frame {
    float                          clear[4];
    uint32_t                       number;
    uint32_t                       index;
    uint32_t                       image_index;
    VkFramebuffer                  framebuffer;
    struct vulkano_per_frame_state state;
};

struct vulkano_swapchain {
    VkSwapchainKHR handle;
    VkRenderPass   render_pass;
    VkExtent2D     extent;
    uint32_t       image_count;

    VkImageView*          image_views;
    struct vulkano_image* depth_images;
    VkImageView*          depth_image_views;
    VkFramebuffer*        framebuffers;
};

struct vulkano_gpu {
    VkPhysicalDevice                 handle;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties       properties;

    VkSurfaceFormatKHR configured_surface_format;
    VkPresentModeKHR   configured_present_mode;

    uint32_t      graphics_queue_family;
    VkQueue       graphics_queue;
    VkCommandPool single_use_command_pool;
};

struct vulkano {
    VkInstance   instance;
    VkSurfaceKHR surface;
    VkDevice     device;

    query_size_function query_size;

    struct vulkano_gpu              gpu;
    struct vulkano_swapchain        swapchain;
    struct vulkano_per_frame_state* frame_state;

    uint32_t frame_counter;
};

typedef enum vulkano_error {
    VULKANO_ERROR_CODE_SUCCESS = 0,
    VULKANO_ERROR_CODE_OUT_OF_MEMORY,
    VULKANO_ERROR_CODE_MINIMIZED,
    VULKANO_ERROR_CODE_UNSUPPORTED_VALIDATION_LAYER,
    VULKANO_ERROR_CODE_UNSUPPORTED_INSTANCE_EXTENSION,
    VULKANO_ERROR_CODE_NO_SUITABLE_GPU_AVAILABLE,
    VULKANO_ERROR_CODE_INVALID_SWAPCHAIN_IMAGE_COUNT,
    VULKANO_ERROR_CODE_FATAL_ERROR,
} VulkanoError;

struct vulkano_pipeline_config {
    const void*                            next;
    VkPipelineCreateFlags                  flags;
    uint32_t                               stage_count;
    VkPipelineShaderStageCreateInfo        stages[5];
    VkPipelineVertexInputStateCreateInfo   vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkPipelineTessellationStateCreateInfo  tessellation_state;
    VkPipelineViewportStateCreateInfo      viewport_state;
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    VkPipelineMultisampleStateCreateInfo   multisample_state;
    VkPipelineDepthStencilStateCreateInfo  depth_stencil_state;
    VkPipelineColorBlendStateCreateInfo    color_blend_state;
    VkPipelineDynamicStateCreateInfo       dynamic_state;
    VkPipelineLayout                       layout;
    VkRenderPass                           render_pass;
    uint32_t                               subpass;
    VkPipeline                             base_pipeline_handle;
    int32_t                                base_pipeline_index;
};

struct vulkano vulkano_create(struct vulkano_config, VulkanoError*);
void           vulkano_destroy(struct vulkano*);

#ifdef VULKANO_INTEGRATE_SDL
// title, width, height have defaults
struct sdl_config {
    const char* title;
    int         left;
    int         top;
    int         width;
    int         height;
    uint32_t    window_flags;
    uint32_t    init_flags;
};

struct vulkano_sdl {
    struct vulkano     vk;
    struct SDL_Window* sdl;
};

// clang-format off

struct vulkano_sdl vulkano_sdl_create(struct vulkano_config, struct sdl_config, VulkanoError*);
void               vulkano_sdl_destroy(struct vulkano_sdl*);
struct vulkano     vulkano_from_sdl(struct vulkano_config, struct SDL_Window*, VulkanoError*);
#endif

void vulkano_configure_swapchain(struct vulkano*, VkRenderPass, uint32_t image_count, VulkanoError*);
void vulkano_frame_acquire(struct vulkano*, struct vulkano_frame*, VulkanoError*);
// extra semaphores/fences can be supplied with VkSubmitInfo, (VkSubmitInfo){0} is sufficient in simple cases
void vulkano_frame_submit(struct vulkano*, struct vulkano_frame*, VkSubmitInfo, VulkanoError*);

struct vulkano_buffer vulkano_buffer_create(struct vulkano*, VkBufferCreateInfo, VkMemoryPropertyFlags, VulkanoError*);
void                  vulkano_buffer_destroy(struct vulkano*, struct vulkano_buffer*);
void                  vulkano_buffer_copy_to(struct vulkano*, struct vulkano_buffer*, struct vulkano_data, VulkanoError*);

struct vulkano_image vulkano_image_create(struct vulkano*, VkImageCreateInfo, VkMemoryPropertyFlags, VulkanoError*);
void                 vulkano_image_destroy(struct vulkano*, struct vulkano_image*);
void                 vulkano_image_copy_to(struct vulkano*, struct vulkano_image*, struct vulkano_image_data, VulkanoError*);
void                 vulkano_image_change_layout(struct vulkano*, struct vulkano_image*, VkImageLayout, VulkanoError*);

struct vulkano_sampler vulkano_sampler_create(struct vulkano*, struct vulkano_image_data, VulkanoError*);
void                   vulkano_sampler_destroy(struct vulkano*, struct vulkano_sampler*);

VkCommandBuffer vulkano_acquire_single_use_command_buffer(struct vulkano*, VulkanoError*);
void            vulkano_submit_single_use_command_buffer(struct vulkano*, VkCommandBuffer, VulkanoError*);

VkPipelineLayout      vulkano_create_pipeline_layout(struct vulkano*, VkPipelineLayoutCreateInfo, VulkanoError*);
VkDescriptorSetLayout vulkano_create_descriptor_set_layout(struct vulkano*, VkDescriptorSetLayoutCreateInfo, VulkanoError*);
VkDescriptorPool      vulkano_create_descriptor_pool(struct vulkano*, VkDescriptorPoolCreateInfo, VulkanoError*);
VkPipeline            vulkano_create_graphics_pipeline(struct vulkano*, struct vulkano_pipeline_config, VulkanoError*);

VkShaderModule vulkano_create_shader_module(struct vulkano*, struct vulkano_data, VulkanoError*);
VkRenderPass   vulkano_create_render_pass(struct vulkano*, VkRenderPassCreateInfo, VulkanoError*);
VkCommandPool  vulkano_create_command_pool(struct vulkano*, VkCommandPoolCreateInfo, VulkanoError*);
VkImageView    vulkano_create_image_view(struct vulkano*, VkImageViewCreateInfo, VulkanoError*);
VkSampler      vulkano_create_sampler(struct vulkano*, VkSamplerCreateInfo, VulkanoError*);
VkSemaphore    vulkano_create_semaphore(struct vulkano*, VkSemaphoreCreateInfo, VulkanoError*);
VkFence        vulkano_create_fence(struct vulkano*, VkFenceCreateInfo, VulkanoError*);

void vulkano_allocate_command_buffers(struct vulkano*, VkCommandBufferAllocateInfo, VkCommandBuffer[], VulkanoError*);
void vulkano_allocate_descriptor_sets(struct vulkano*, VkDescriptorSetAllocateInfo, VkDescriptorSet[], VulkanoError*);

// clang-format on

#define VULKANO_WIDTH(vulkano) (vulkano)->swapchain.extent.width
#define VULKANO_HEIGHT(vulkano) (vulkano)->swapchain.extent.height
#define VULKANO_VIEWPORT(vulkano)                                              \
    (VkViewport)                                                               \
    {                                                                          \
        .width  = (float)VULKANO_WIDTH(vulkano),                               \
        .height = (float)VULKANO_HEIGHT(vulkano), .maxDepth = 1.0f             \
    }
#define VULKANO_SCISSOR(vulkano)                                               \
    (VkRect2D) { .extent = (vulkano)->swapchain.extent }

const char* vkresult_to_string(VkResult);

void vulkano_log(
    const char* label, const char* filepath, int line, const char* message
);
void vulkano_logf(
    const char* label, const char* filepath, int line, const char* fmt, ...
);

#define VULKANO_INFO(message)                                                  \
    vulkano_log("[INFO]", __FILE__, __LINE__, message);
#define VULKANO_INFOF(fmt, ...)                                                \
    vulkano_logf("[INFO]", __FILE__, __LINE__, fmt, __VA_ARGS__);
#define VULKANO_ERROR(message)                                                 \
    vulkano_log("[ERROR]", __FILE__, __LINE__, message);
#define VULKANO_ERRORF(fmt, ...)                                               \
    vulkano_logf("[ERROR]", __FILE__, __LINE__, fmt, __VA_ARGS__);

#define VULKANO_CHECK(expression, error)                                       \
    do {                                                                       \
        if (*error) break;                                                     \
        VkResult check_result = expression;                                    \
        if (check_result == VK_ERROR_OUT_OF_HOST_MEMORY ||                     \
            check_result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {                   \
            VULKANO_ERRORF(                                                    \
                "out of memory (%s)\n", vkresult_to_string(check_result)       \
            );                                                                 \
            *error = VULKANO_ERROR_CODE_OUT_OF_MEMORY;                         \
        }                                                                      \
        else if (check_result != VK_SUCCESS) {                                 \
            VULKANO_ERRORF(                                                    \
                "fatal error encountered (%s)\n",                              \
                vkresult_to_string(check_result)                               \
            );                                                                 \
            *error = VULKANO_ERROR_CODE_FATAL_ERROR;                           \
        }                                                                      \
    } while (0)

//
// **************************************************************************************
//
//                               BEGIN IMPLEMENTATION
//
// **************************************************************************************
//

#ifdef VULKANO_IMPLEMENTATION

void
vulkano_log(
    const char* label, const char* filepath, int line, const char* message
)
{
#ifndef VULKANO_DISABLE_LOG
    fprintf(VULKANO_LOG_FILE, "%s (%s:%i): %s", label, filepath, line, message);
#endif
}

void
vulkano_logf(
    const char* label, const char* filepath, int line, const char* fmt, ...
)
{
#ifndef VULKANO_DISABLE_LOG
    va_list args;
    va_start(args, fmt);
    fprintf(VULKANO_LOG_FILE, "%s (%s:%i): ", label, filepath, line);
    vfprintf(VULKANO_LOG_FILE, fmt, args);
    va_end(args);
#endif
}

#define VULKANO_CLAMP(min, max, value)                                         \
    ((value < min) ? min : ((value > max) ? max : value))

typedef int (*compare_function)(const void*, const void*);

// forward declare helpers

struct string_array {
    uint32_t     count;
    const char** data;
};

struct {
    size_t count;
    size_t capacity;
    void** allocations;
} init_allocations = {0};

// used by initialization functions. these allocations will be freed at
// the end of initialization
static void* init_malloc(size_t);
static void  free_init_allocations(void);

#define VULKANO_INIT_MALLOC_ARRAY(pointer, capacity)                           \
    pointer = init_malloc((sizeof *(pointer)) * capacity)

static struct string_array combine_string_arrays_unique(
    struct string_array array1, struct string_array array2
);

static const char* present_mode_to_string(VkPresentModeKHR mode);
static const char* color_format_to_string(VkFormat fmt);
static const char* color_space_to_string(VkColorSpaceKHR space);
static const char* gpu_name(VkPhysicalDevice gpu);
static int         default_gpu_compare(VkPhysicalDevice*, VkPhysicalDevice*);
static int
default_surface_format_compare(VkSurfaceFormatKHR*, VkSurfaceFormatKHR*);
static int default_present_modes_compare(VkPresentModeKHR*, VkPresentModeKHR*);
static int compare_extension_names(
    const VkExtensionProperties* ext1, const VkExtensionProperties* ext2
);
static int compare_layer_properties_name(
    VkLayerProperties* prop1, VkLayerProperties* prop2
);

#define DEFAULT0(value, default_value)                                         \
    (value) = ((value)) ? (value) : (default_value)

static void
create_instance(
    struct vulkano* vk,
    uint32_t        validation_layers_count,
    const char**    validation_layers,
    uint32_t        extensions_count,
    const char**    extensions,
    VulkanoError*   error
)
{
    if (*error) return;

#ifdef VULKANO_DISABLE_VALIDATION_LAYERS
    validation_layers_count = 0;
    validation_layers       = NULL;
#endif

    VULKANO_INFO("required instance extensions:\n");
    for (uint32_t i = 0; i < extensions_count; i++)
        VULKANO_INFOF("  %s\n", extensions[i]);
    VULKANO_INFO("\n");

    VULKANO_INFO("required instance validation layers:\n");
    for (uint32_t i = 0; i < validation_layers_count; i++)
        VULKANO_INFOF("  %s\n", validation_layers[i]);
    VULKANO_INFO("\n");

    // enumerate available validation layers
    uint32_t available_layers_count = 1;
    VULKANO_CHECK(
        vkEnumerateInstanceLayerProperties(&available_layers_count, NULL), error
    );
    VkLayerProperties* available_layers;
    VULKANO_INIT_MALLOC_ARRAY(available_layers, available_layers_count);
    VULKANO_CHECK(
        vkEnumerateInstanceLayerProperties(
            &available_layers_count, available_layers
        ),
        error
    );
    if (*error) return;

    // check that required layers are supported
    qsort(
        available_layers,
        available_layers_count,
        sizeof(VkLayerProperties),
        (compare_function)compare_layer_properties_name
    );
    for (uint32_t i = 0; i < validation_layers_count; i++) {
        VkLayerProperties requested = {0};
        snprintf(
            requested.layerName,
            sizeof(requested.layerName),
            "%s",
            validation_layers[i]
        );
        if (bsearch(
                &requested,
                available_layers,
                available_layers_count,
                sizeof(VkLayerProperties),
                (compare_function)compare_layer_properties_name
            ) == NULL) {
            *error = VULKANO_ERROR_CODE_UNSUPPORTED_VALIDATION_LAYER;
            VULKANO_ERRORF(
                "unsupported vulkan validation layer: %s\n",
                validation_layers[i]
            );
        }
    }
    if (*error) return;

    // enumerate available instance extensions
    uint32_t supported_count = 1;
    VULKANO_CHECK(
        vkEnumerateInstanceExtensionProperties(NULL, &supported_count, NULL),
        error
    );
    VkExtensionProperties* supported_extensions;
    VULKANO_INIT_MALLOC_ARRAY(supported_extensions, supported_count);
    VULKANO_CHECK(
        vkEnumerateInstanceExtensionProperties(
            NULL, &supported_count, supported_extensions
        ),
        error
    );
    if (*error) return;

    // ensure extensions are available
    qsort(
        supported_extensions,
        supported_count,
        sizeof(VkExtensionProperties),
        (compare_function)compare_extension_names
    );
    for (uint32_t i = 0; i < extensions_count; i++) {
        VkExtensionProperties required_extension = {0};
        snprintf(
            required_extension.extensionName,
            sizeof(required_extension.extensionName),
            "%s",
            extensions[i]
        );
        if (bsearch(
                &required_extension,
                supported_extensions,
                supported_count,
                sizeof(required_extension),
                (compare_function)compare_extension_names
            ) == NULL) {
            *error = VULKANO_ERROR_CODE_UNSUPPORTED_INSTANCE_EXTENSION;
            VULKANO_ERRORF(
                "unsupported instance extension: %s\n", extensions[i]
            );
        }
    }
    if (*error) return;

    // create instance
    VkInstanceCreateInfo vk_create_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount       = validation_layers_count,
        .ppEnabledLayerNames     = validation_layers,
        .enabledExtensionCount   = extensions_count,
        .ppEnabledExtensionNames = extensions,
    };
    VULKANO_CHECK(
        vkCreateInstance(&vk_create_info, NULL, &vk->instance), error
    );
    if (*error) return;
}

static VkPresentModeKHR
select_present_mode(
    VkPhysicalDevice              gpu,
    VkSurfaceKHR                  surface,
    present_mode_compare_function cmp,
    VulkanoError*                 error
)
{
    if (*error) return (VkPresentModeKHR)0;

    // query capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities),
        error
    );
    if (*error) return (VkPresentModeKHR)0;

    // enumerate available present modes
    uint32_t present_modes_count = 1;
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            gpu, surface, &present_modes_count, NULL
        ),
        error
    );
    VkPresentModeKHR* present_modes;
    VULKANO_INIT_MALLOC_ARRAY(present_modes, present_modes_count);
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            gpu, surface, &present_modes_count, present_modes
        ),
        error
    );
    if (*error) return (VkPresentModeKHR)0;

    // select best mode
    qsort(  // sorted worst->best
        present_modes,
        present_modes_count,
        sizeof(VkPresentModeKHR),
        (compare_function)cmp
    );
    VkPresentModeKHR present_mode = present_modes[present_modes_count - 1];
    return present_mode;
}

static VkSurfaceFormatKHR
select_surface_format(
    VkPhysicalDevice                gpu,
    VkSurfaceKHR                    surface,
    surface_format_compare_function cmp,
    VulkanoError*                   error
)
{
    if (*error) return (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};

    // query device capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities),
        error
    );
    if (*error) return (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};

    // enumerate available formats
    uint32_t formats_count = 1;
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            gpu, surface, &formats_count, NULL
        ),
        error
    );
    VkSurfaceFormatKHR* surface_formats;
    VULKANO_INIT_MALLOC_ARRAY(surface_formats, formats_count);
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            gpu, surface, &formats_count, surface_formats
        ),
        error
    );
    if (*error) return (VkSurfaceFormatKHR){.format = VK_FORMAT_UNDEFINED};

    // select best format
    qsort(  // sorted worst->best
        surface_formats,
        formats_count,
        sizeof(VkSurfaceFormatKHR),
        (compare_function)cmp
    );
    VkSurfaceFormatKHR surface_format = surface_formats[formats_count - 1];
    return surface_format;
}

static bool
confirm_gpu_selection(
    VkSurfaceKHR        surface,
    struct vulkano_gpu* gpu,
    uint32_t            extension_count,
    const char**        extensions
)
{
    VulkanoError error = 0;

    // enumerate supported extensions
    uint32_t supported_extensions_count = 1;
    VULKANO_CHECK(
        vkEnumerateDeviceExtensionProperties(
            gpu->handle, NULL, &supported_extensions_count, NULL
        ),
        &error
    );
    VkExtensionProperties* supported_extensions;
    VULKANO_INIT_MALLOC_ARRAY(supported_extensions, supported_extensions_count);
    VULKANO_CHECK(
        vkEnumerateDeviceExtensionProperties(
            gpu->handle, NULL, &supported_extensions_count, supported_extensions
        ),
        &error
    );
    if (error) {
        VULKANO_INFO("unable to enumerate gpu supported extensions\n");
        return false;
    }

    // ensure all extensions are supported
    qsort(
        supported_extensions,
        supported_extensions_count,
        sizeof(VkExtensionProperties),
        (compare_function)compare_extension_names
    );
    bool supported = true;
    for (size_t i = 0; i < extension_count; i++) {
        VkExtensionProperties required_extension = {0};
        snprintf(
            required_extension.extensionName,
            sizeof(required_extension.extensionName),
            "%s",
            extensions[i]
        );
        if (!bsearch(
                &required_extension,
                supported_extensions,
                supported_extensions_count,
                sizeof(VkExtensionProperties),
                (compare_function)compare_extension_names
            )) {
            supported = false;
            VULKANO_ERRORF("unsupported gpu extension: %s\n", extensions[i]);
        }
    }
    if (!supported) {
        VULKANO_ERRORF(
            "  GPU: [%s] failed requirements check: does not support all "
            "required "
            "extensions\n",
            gpu_name(gpu->handle)
        );
        return false;
    }

    // TODO: make this configurable
    //
    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(gpu->handle, &supported_features);
    if (!supported_features.samplerAnisotropy) {
        VULKANO_ERRORF(
            "  GPU: [%s] failed requirements check: does not support sampler "
            "anisotropy\n",
            gpu_name(gpu->handle)
        );
    }

    // search for queue family that supports graphics & presentation to our
    // surface
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(
        gpu->handle, &queue_family_count, NULL
    );
    VkQueueFamilyProperties* queue_family_properties;
    VULKANO_INIT_MALLOC_ARRAY(queue_family_properties, queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(
        gpu->handle, &queue_family_count, queue_family_properties
    );

    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkBool32 presentation_supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            gpu->handle, i, surface, &presentation_supported
        );
        bool suitable_device =
            presentation_supported &&
            queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        // if a suitable device is found fill selected queue family information
        // on the gpu struct and return true
        if (suitable_device) {
            gpu->graphics_queue_family = i;
            vkGetPhysicalDeviceMemoryProperties(
                gpu->handle, &gpu->memory_properties
            );
            vkGetPhysicalDeviceProperties(gpu->handle, &gpu->properties);
            return true;
        }
    }

    VULKANO_ERRORF(
        "  GPU: [%s] failed requirements check: does not support all required "
        "queues\n",
        gpu_name(gpu->handle)
    );
    return false;
}

static void
select_gpu(
    struct vulkano*                 vk,
    gpu_compare_function            gpucmp,
    present_mode_compare_function   presentcmp,
    surface_format_compare_function fmtcmp,
    uint32_t                        extensions_count,
    const char**                    extensions,
    VulkanoError*                   error
)
{
    if (*error) return;

    VULKANO_INFO("selecting gpu\n");
    for (uint32_t i = 0; i < extensions_count; i++)
        VULKANO_INFOF("  required extension: %s\n", extensions[i]);

    // enumerate available devices
    uint32_t device_count = 0;
    VULKANO_CHECK(
        vkEnumeratePhysicalDevices(vk->instance, &device_count, NULL), error
    );
    if (device_count == 0) {
        VULKANO_ERROR("no gpus visible to vulkan\n");
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        return;
    }
    VkPhysicalDevice* devices;
    VULKANO_INIT_MALLOC_ARRAY(devices, device_count);
    VULKANO_CHECK(
        vkEnumeratePhysicalDevices(vk->instance, &device_count, devices), error
    );
    if (*error) return;

    qsort(
        devices,
        device_count,
        sizeof(VkPhysicalDevice),
        (compare_function)gpucmp
    );
    VULKANO_INFO(
        "  available gpus listed worst->best by configured ranking function:\n"
    );
    for (uint32_t i = 0; i < device_count; i++)
        VULKANO_INFOF("    %s\n", gpu_name(devices[i]));

    // iterate devices from best->worst and return the first device that
    // meets the requirements
    //
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDevice gpu = devices[device_count - 1 - i];
        vk->gpu.handle       = gpu;
        vk->gpu.configured_present_mode =
            select_present_mode(gpu, vk->surface, presentcmp, error);
        vk->gpu.configured_surface_format =
            select_surface_format(gpu, vk->surface, fmtcmp, error);
        if (*error) return;

        if (confirm_gpu_selection(
                vk->surface, &vk->gpu, extensions_count, extensions
            )) {
            VULKANO_INFOF(
                "  configured present mode %s\n",
                present_mode_to_string(vk->gpu.configured_present_mode)
            );
            VULKANO_INFOF(
                "  configured surface format with color format %s\n",
                color_format_to_string(vk->gpu.configured_surface_format.format)
            );
            VULKANO_INFOF(
                "  configured surface format with color space %s\n",
                color_space_to_string(
                    vk->gpu.configured_surface_format.colorSpace
                )
            );
            VULKANO_INFOF("selected gpu: %s\n\n", gpu_name(vk->gpu.handle));
            return;
        }
    }

    *error = VULKANO_ERROR_CODE_NO_SUITABLE_GPU_AVAILABLE;
    return;
}

static void
create_device(
    struct vulkano* vk,
    uint32_t        gpu_extensions_count,
    const char**    gpu_extensions,
    VulkanoError*   error
)
{
    if (*error) return;

    VkPhysicalDeviceFeatures gpu_features = {.samplerAnisotropy = VK_TRUE};

    float                   queue_priorities[]   = {1.0};
    VkDeviceQueueCreateInfo queue_create_infos[] = {
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount       = 1,
            .pQueuePriorities = queue_priorities,
        },
    };
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount =
            sizeof(queue_create_infos) / sizeof(queue_create_infos[0]),
        .pQueueCreateInfos       = queue_create_infos,
        .enabledExtensionCount   = gpu_extensions_count,
        .ppEnabledExtensionNames = gpu_extensions,
        .pEnabledFeatures        = &gpu_features,
    };

    VULKANO_CHECK(
        vkCreateDevice(vk->gpu.handle, &create_info, NULL, &vk->device), error
    );
    if (*error) return;

    vkGetDeviceQueue(
        vk->device, vk->gpu.graphics_queue_family, 0, &vk->gpu.graphics_queue
    );
    vk->gpu.single_use_command_pool = vulkano_create_command_pool(
        vk,
        (VkCommandPoolCreateInfo
        ){.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT},
        error
    );
    if (*error) return;
}

struct vulkano
vulkano_create(struct vulkano_config config, VulkanoError* error)
{
    struct vulkano vk            = {0};
    const char*    surface_error = NULL;

    if (!config.surface_creation) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("vulkano_config.surface_creation must be specified");
        return vk;
    }
    if (!config.query_window_size) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("vulkano_config.query_window_size must be specified");
        return vk;
    }

    vk.query_size = config.query_window_size;

    DEFAULT0(config.gpu_compare, default_gpu_compare);
    DEFAULT0(config.format_compare, default_surface_format_compare);
    DEFAULT0(config.present_compare, default_present_modes_compare);

    // various default configurations availble through preprocessor
    static const char* DEFAULT_INSTANCE_VALIDATION_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"};
    static const char* DEFAULT_GRAPHICS_GPU_EXTENSIONS[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    static struct string_array LIBRARY_REQUIRED_GPU_EXTENSIONS = {
        .count = 0,
        .data  = NULL,
    };
    static struct string_array LIBRARY_REQUIRED_INSTANCE_EXTENSIONS = {
        .count = 0,
        .data  = NULL,
    };
    static struct string_array LIBRARY_REQUIRED_VALIDATION_LAYERS = {
        .count = 0,
        .data  = NULL,
    };
#ifdef VULKANO_ENABLE_DEFAULT_VALIDATION_LAYERS
    LIBRARY_REQUIRED_VALIDATION_LAYERS.data =
        DEFAULT_INSTANCE_VALIDATION_LAYERS;
    LIBRARY_REQUIRED_VALIDATION_LAYERS.count =
        sizeof(DEFAULT_INSTANCE_VALIDATION_LAYERS) / sizeof(const char*);
#endif
#ifdef VULKANO_ENABLE_DEFAULT_GRAPHICS_EXTENSIONS
    LIBRARY_REQUIRED_GPU_EXTENSIONS.data = DEFAULT_GRAPHICS_GPU_EXTENSIONS;
    LIBRARY_REQUIRED_GPU_EXTENSIONS.count =
        sizeof(DEFAULT_GRAPHICS_GPU_EXTENSIONS) / sizeof(const char*);
#endif

    // combine defaults (if requested) and user requested configuration into
    // single arrays with duplicates stripped
    //
    struct string_array user_required_validation_layers = {
        .count = config.validation_layers_count,
        .data  = config.validation_layers,
    };
    struct string_array user_required_instance_extensions = {
        .count = config.instance_extensions_count,
        .data  = config.instance_extensions,
    };
    struct string_array user_required_gpu_extensions = {
        .count = config.gpu_extensions_count,
        .data  = config.gpu_extensions,
    };
    struct string_array required_validation_layers =
        combine_string_arrays_unique(
            LIBRARY_REQUIRED_VALIDATION_LAYERS, user_required_validation_layers
        );
    struct string_array required_instance_extensions =
        combine_string_arrays_unique(
            LIBRARY_REQUIRED_INSTANCE_EXTENSIONS,
            user_required_instance_extensions
        );
    struct string_array required_gpu_extensions = combine_string_arrays_unique(
        LIBRARY_REQUIRED_GPU_EXTENSIONS, user_required_gpu_extensions
    );
    if (*error) goto cleanup;

    VULKANO_INFO("INITIALIZING VULKAN\n\n");
    VULKANO_INFO("required instance extensions:\n");
    for (uint32_t i = 0; i < required_instance_extensions.count; i++)
        VULKANO_INFOF("  %s\n", required_instance_extensions.data[i]);
    VULKANO_INFO("\n");

    VULKANO_INFO("required instance validation layers:\n");
    for (uint32_t i = 0; i < required_validation_layers.count; i++)
        VULKANO_INFOF("  %s\n", required_validation_layers.data[i]);
    VULKANO_INFO("\n");

    create_instance(
        &vk,
        required_validation_layers.count,
        required_validation_layers.data,
        required_instance_extensions.count,
        required_instance_extensions.data,
        error
    );
    if (*error) goto cleanup;

    // create surface
    // native surface creation not currently implemented
    // TODO: implement native windows?
    surface_error = config.surface_creation(vk.instance, &vk.surface);
    if (surface_error) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(surface_error);
        goto cleanup;
    }

    select_gpu(
        &vk,
        config.gpu_compare,
        config.present_compare,
        config.format_compare,
        required_gpu_extensions.count,
        required_gpu_extensions.data,
        error
    );
    create_device(
        &vk, required_gpu_extensions.count, required_gpu_extensions.data, error
    );
    if (*error) goto cleanup;

cleanup:
    free_init_allocations();
    if (*error) vulkano_destroy(&vk);
    return vk;
}

static void
destroy_per_frame_state(struct vulkano* vk)
{
    if (!vk->device || !vk->frame_state) return;
    vkDeviceWaitIdle(vk->device);

    for (uint32_t i = 0; i < vk->swapchain.image_count; i++) {
        vkDestroyCommandPool(vk->device, vk->frame_state[i].command_pool, NULL);
        vkDestroySemaphore(
            vk->device, vk->frame_state[i].image_ready_for_use, NULL
        );
        vkDestroySemaphore(
            vk->device, vk->frame_state[i].rendering_commands_complete, NULL
        );
        vkDestroyFence(
            vk->device, vk->frame_state[i].presentation_complete, NULL
        );
    }
    free(vk->frame_state);
    vk->frame_state = NULL;
}

static void
create_per_frame_state(struct vulkano* vk, VulkanoError* error)
{
    if (*error) return;

    assert(
        vk->swapchain.image_count &&
        "swapchain needs to be setup before this call"
    );
    destroy_per_frame_state(vk);

    vk->frame_state =
        calloc(vk->swapchain.image_count, sizeof *vk->frame_state);
    if (!vk->frame_state) {
        *error = VULKANO_ERROR_CODE_OUT_OF_MEMORY;
        VULKANO_ERROR("out of memory\n");
        return;
    }

    for (uint32_t i = 0; i < vk->swapchain.image_count; i++) {
        vk->frame_state[i].image_ready_for_use =
            vulkano_create_semaphore(vk, (VkSemaphoreCreateInfo){0}, error);
        vk->frame_state[i].rendering_commands_complete =
            vulkano_create_semaphore(vk, (VkSemaphoreCreateInfo){0}, error);
        vk->frame_state[i].presentation_complete = vulkano_create_fence(
            vk,
            (VkFenceCreateInfo){.flags = VK_FENCE_CREATE_SIGNALED_BIT},
            error
        );
        vk->frame_state[i].command_pool = vulkano_create_command_pool(
            vk,
            (VkCommandPoolCreateInfo
            ){.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT},
            error
        );
        vulkano_allocate_command_buffers(
            vk,
            (VkCommandBufferAllocateInfo){
                .commandPool        = vk->frame_state[i].command_pool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            },
            &vk->frame_state[i].render_command,
            error
        );
        if (*error) {
            destroy_per_frame_state(vk);
            return;
        }
    }
}

static void
destroy_swapchain(struct vulkano* vk)
{
    if (!vk->device) return;
    vkDeviceWaitIdle(vk->device);
    for (uint32_t i = 0; i < vk->swapchain.image_count; i++) {
        if (vk->swapchain.image_views)
            vkDestroyImageView(vk->device, vk->swapchain.image_views[i], NULL);
        if (vk->swapchain.depth_image_views)
            vkDestroyImageView(
                vk->device, vk->swapchain.depth_image_views[i], NULL
            );
        if (vk->swapchain.depth_images)
            vulkano_image_destroy(vk, &vk->swapchain.depth_images[i]);
        if (vk->swapchain.framebuffers)
            vkDestroyFramebuffer(
                vk->device, vk->swapchain.framebuffers[i], NULL
            );
    }
    free(vk->swapchain.image_views);
    free(vk->swapchain.depth_image_views);
    free(vk->swapchain.depth_images);
    free(vk->swapchain.framebuffers);
    vk->swapchain.image_views       = NULL;
    vk->swapchain.depth_image_views = NULL;
    vk->swapchain.depth_images      = NULL;
    vk->swapchain.framebuffers      = NULL;
    vkDestroySwapchainKHR(vk->device, vk->swapchain.handle, NULL);
    vk->swapchain.handle = VK_NULL_HANDLE;
}

void
vulkano_destroy(struct vulkano* vk)
{
    if (vk->device) vkDeviceWaitIdle(vk->device);
    destroy_per_frame_state(vk);
    destroy_swapchain(vk);
    vkDestroyCommandPool(vk->device, vk->gpu.single_use_command_pool, NULL);
    if (vk->device) vkDestroyDevice(vk->device, NULL);
    if (vk->surface) vkDestroySurfaceKHR(vk->instance, vk->surface, NULL);
    if (vk->instance) vkDestroyInstance(vk->instance, NULL);
    *vk = (struct vulkano){0};
}

VkCommandPool
vulkano_create_command_pool(
    struct vulkano* vk, VkCommandPoolCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    info.sType         = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    VkCommandPool pool = VK_NULL_HANDLE;
    VULKANO_CHECK(vkCreateCommandPool(vk->device, &info, NULL, &pool), error);
    return pool;
}

VkImageView
vulkano_create_image_view(
    struct vulkano* vk, VkImageViewCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    DEFAULT0(info.viewType, VK_IMAGE_VIEW_TYPE_2D);
    DEFAULT0(info.format, vk->gpu.configured_surface_format.format);
    DEFAULT0(info.subresourceRange.aspectMask, VK_IMAGE_ASPECT_COLOR_BIT);
    DEFAULT0(info.subresourceRange.levelCount, 1);
    DEFAULT0(info.subresourceRange.layerCount, 1);
    VkImageView image_view = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateImageView(vk->device, &info, NULL, &image_view), error
    );
    return image_view;
}

VkSampler
vulkano_create_sampler(
    struct vulkano* vk, VkSamplerCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    VkSampler sampler = VK_NULL_HANDLE;
    VULKANO_CHECK(vkCreateSampler(vk->device, &info, NULL, &sampler), error);
    return sampler;
}

VkSemaphore
vulkano_create_semaphore(
    struct vulkano* vk, VkSemaphoreCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateSemaphore(vk->device, &info, NULL, &semaphore), error
    );
    return semaphore;
}

VkFence
vulkano_create_fence(
    struct vulkano* vk, VkFenceCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    VkFence fence = VK_NULL_HANDLE;
    VULKANO_CHECK(vkCreateFence(vk->device, &info, NULL, &fence), error);
    return fence;
}

void
vulkano_allocate_command_buffers(
    struct vulkano*             vk,
    VkCommandBufferAllocateInfo info,
    VkCommandBuffer*            command_buffers,
    VulkanoError*               error
)
{
    if (*error) return;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    DEFAULT0(info.commandBufferCount, 1);
    VULKANO_CHECK(
        vkAllocateCommandBuffers(vk->device, &info, command_buffers), error
    );
}

void
vulkano_allocate_descriptor_sets(
    struct vulkano*             vk,
    VkDescriptorSetAllocateInfo info,
    VkDescriptorSet             sets[],
    VulkanoError*               error
)
{
    if (*error) return;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    VULKANO_CHECK(vkAllocateDescriptorSets(vk->device, &info, sets), error);
}

VkRenderPass
vulkano_create_render_pass(
    struct vulkano* vk, VkRenderPassCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;

    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

    // if attachment format is undefined use swapchain format instead
    for (uint32_t attachment = 0; attachment < info.attachmentCount;
         attachment++) {
        VkAttachmentDescription* desc =
            (VkAttachmentDescription*)info.pAttachments + attachment;
        DEFAULT0(desc->format, vk->gpu.configured_surface_format.format);
    }

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateRenderPass(vk->device, &info, NULL, &render_pass), error
    );
    return render_pass;
}

VkDescriptorSetLayout
vulkano_create_descriptor_set_layout(
    struct vulkano*                 vk,
    VkDescriptorSetLayoutCreateInfo info,
    VulkanoError*                   error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateDescriptorSetLayout(vk->device, &info, NULL, &layout), error
    );
    return layout;
}

VkPipelineLayout
vulkano_create_pipeline_layout(
    struct vulkano* vk, VkPipelineLayoutCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    DEFAULT0(info.sType, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreatePipelineLayout(vk->device, &info, NULL, &layout), error
    );
    return layout;
}

VkShaderModule
vulkano_create_shader_module(
    struct vulkano* vk, struct vulkano_data data, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = data.size,
        .pCode    = (uint32_t*)data.data,
    };
    VkShaderModule module = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateShaderModule(vk->device, &info, NULL, &module), error
    );
    return module;
}

VkPipeline
vulkano_create_graphics_pipeline(
    struct vulkano* vk, struct vulkano_pipeline_config info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;

    for (size_t i = 0; i < info.stage_count; i++) {
        info.stages[i].sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        DEFAULT0(info.stages[i].pName, "main");
    }
    info.vertex_input_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.input_assembly_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.tessellation_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    info.viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.rasterization_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.multisample_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.depth_stencil_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.color_blend_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.dynamic_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    DEFAULT0(
        info.multisample_state.rasterizationSamples, VK_SAMPLE_COUNT_1_BIT
    );
    VkGraphicsPipelineCreateInfo processed_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags               = info.flags,
        .stageCount          = info.stage_count,
        .pStages             = info.stages,
        .pVertexInputState   = &info.vertex_input_state,
        .pInputAssemblyState = &info.input_assembly_state,
        .pTessellationState  = &info.tessellation_state,
        .pViewportState      = &info.viewport_state,
        .pRasterizationState = &info.rasterization_state,
        .pMultisampleState   = &info.multisample_state,
        .pDepthStencilState  = &info.depth_stencil_state,
        .pColorBlendState    = &info.color_blend_state,
        .pDynamicState       = &info.dynamic_state,
        .layout              = info.layout,
        .renderPass          = info.render_pass,
        .subpass             = info.subpass,
        .basePipelineHandle  = info.base_pipeline_handle,
        .basePipelineIndex   = info.base_pipeline_index,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    VULKANO_CHECK(
        vkCreateGraphicsPipelines(
            vk->device, NULL, 1, &processed_info, NULL, &pipeline
        ),
        error
    );
    return pipeline;
}

VkDescriptorPool
vulkano_create_descriptor_pool(
    struct vulkano* vk, VkDescriptorPoolCreateInfo info, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;
    info.sType            = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    ;
    VULKANO_CHECK(
        vkCreateDescriptorPool(vk->device, &info, NULL, &pool), error
    );
    return pool;
}

static void
create_swapchain(struct vulkano* vk, VulkanoError* error)
{
    if (*error) return;
    destroy_swapchain(vk);

    VkSurfaceCapabilitiesKHR capabilities = {0};
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            vk->gpu.handle, vk->surface, &capabilities
        ),
        error
    );
    if (*error) return;
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFF) {
        uint32_t width, height;
        vk->query_size(&width, &height);
        extent.width = VULKANO_CLAMP(
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width,
            width
        );
        extent.height = VULKANO_CLAMP(
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height,
            height
        );
    }

    if (extent.width == 0 || extent.height == 0) {
        *error = VULKANO_ERROR_CODE_MINIMIZED;
        return;
    }

    vk->swapchain.extent = extent;
    VULKANO_INFOF(
        "creating swapchain with extent (%u, %u)\n",
        vk->swapchain.extent.width,
        vk->swapchain.extent.height
    );

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = vk->surface,
        .minImageCount    = vk->swapchain.image_count,
        .imageFormat      = vk->gpu.configured_surface_format.format,
        .imageColorSpace  = vk->gpu.configured_surface_format.colorSpace,
        .imageExtent      = vk->swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = vk->gpu.configured_present_mode,
        .clipped          = VK_TRUE,
    };
    VULKANO_CHECK(
        vkCreateSwapchainKHR(
            vk->device, &swapchain_info, NULL, &vk->swapchain.handle
        ),
        error
    );
    if (*error) return;

    VkImage* images;
    VULKANO_INIT_MALLOC_ARRAY(images, vk->swapchain.image_count);
    VULKANO_CHECK(
        vkGetSwapchainImagesKHR(
            vk->device, vk->swapchain.handle, &vk->swapchain.image_count, images
        ),
        error
    );
    if (*error) return;

    vk->swapchain.image_views =
        calloc(vk->swapchain.image_count, sizeof *vk->swapchain.image_views);
    vk->swapchain.depth_images =
        calloc(vk->swapchain.image_count, sizeof *vk->swapchain.depth_images);
    vk->swapchain.depth_image_views = calloc(
        vk->swapchain.image_count, sizeof *vk->swapchain.depth_image_views
    );
    vk->swapchain.framebuffers =
        calloc(vk->swapchain.image_count, sizeof *vk->swapchain.framebuffers);

    if (!(vk->swapchain.image_views && vk->swapchain.depth_images &&
          vk->swapchain.depth_image_views && vk->swapchain.framebuffers)) {
        *error = VULKANO_ERROR_CODE_OUT_OF_MEMORY;
        VULKANO_ERROR("out of memory");
        return;
    }

    for (uint32_t i = 0; i < vk->swapchain.image_count; i++) {
        vk->swapchain.image_views[i] = vulkano_create_image_view(
            vk,
            (VkImageViewCreateInfo){
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = vk->gpu.configured_surface_format.format,
                .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel   = 0,
                .subresourceRange.levelCount     = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount     = 1,
            },
            error
        );
        vk->swapchain.depth_images[i] = vulkano_image_create(
            vk,
            (VkImageCreateInfo){
                .format = VULKANO_DEPTH_FORMAT,
                .extent =
                    {vk->swapchain.extent.width,
                     vk->swapchain.extent.height,
                     1},
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices =
                    (const uint32_t[]){vk->gpu.graphics_queue_family},
            },
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            error
        );
        vk->swapchain.depth_image_views[i] = vulkano_create_image_view(
            vk,
            (VkImageViewCreateInfo){
                .image  = vk->swapchain.depth_images[i].handle,
                .format = VULKANO_DEPTH_FORMAT,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            },
            error
        );
        if (*error) {
            destroy_swapchain(vk);
            return;
        }

        VkImageView attachments[] = {
            vk->swapchain.image_views[i], vk->swapchain.depth_image_views[i]};
        VkFramebufferCreateInfo framebuffer_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = vk->swapchain.render_pass,
            .attachmentCount = sizeof attachments / sizeof *attachments,
            .pAttachments    = attachments,
            .width           = vk->swapchain.extent.width,
            .height          = vk->swapchain.extent.height,
            .layers          = 1,
        };
        VULKANO_CHECK(
            vkCreateFramebuffer(
                vk->device,
                &framebuffer_info,
                NULL,
                &vk->swapchain.framebuffers[i]
            ),
            error
        );
        if (*error) {
            destroy_swapchain(vk);
            return;
        }
    }
}

void
vulkano_configure_swapchain(
    struct vulkano* vk,
    VkRenderPass    render_pass,
    uint32_t        image_count,
    VulkanoError*   error
)
{
    if (*error) return;

    vk->swapchain.render_pass = render_pass;
    vk->swapchain.image_count = image_count;

    VkSurfaceCapabilitiesKHR capabilities = {0};
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            vk->gpu.handle, vk->surface, &capabilities
        ),
        error
    );
    if (*error) return;

    if (image_count < capabilities.minImageCount ||
        image_count > capabilities.maxImageCount) {
        *error = VULKANO_ERROR_CODE_INVALID_SWAPCHAIN_IMAGE_COUNT;
        VULKANO_ERRORF(
            "min_image_count: %u max_image_count %u\n",
            capabilities.minImageCount,
            capabilities.maxImageCount
        );
        return;
    }

    create_swapchain(vk, error);
    create_per_frame_state(vk, error);
    return;
}

VkCommandBuffer
vulkano_acquire_single_use_command_buffer(
    struct vulkano* vk, VulkanoError* error
)
{
    if (*error) return VK_NULL_HANDLE;

    VkCommandBuffer             cmd             = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocation_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vk->gpu.single_use_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VULKANO_CHECK(
        vkAllocateCommandBuffers(vk->device, &allocation_info, &cmd), error
    );
    if (*error) return cmd;

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VULKANO_CHECK(vkBeginCommandBuffer(cmd, &begin_info), error);
    if (*error) {
        vkFreeCommandBuffers(
            vk->device, vk->gpu.single_use_command_pool, 1, &cmd
        );
        return VK_NULL_HANDLE;
    }
    return cmd;
}

void
vulkano_submit_single_use_command_buffer(
    struct vulkano* vk, VkCommandBuffer cmd, VulkanoError* error
)
{
    if (*error) return;

    static const VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence = VK_NULL_HANDLE;

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };

    VULKANO_CHECK(vkCreateFence(vk->device, &fence_info, NULL, &fence), error);
    VULKANO_CHECK(vkEndCommandBuffer(cmd), error);
    VULKANO_CHECK(
        vkQueueSubmit(vk->gpu.graphics_queue, 1, &submit, fence), error
    );
    VULKANO_CHECK(
        vkWaitForFences(vk->device, 1, &fence, VK_TRUE, VULKANO_TIMEOUT), error
    );
    if (*error) goto cleanup;

cleanup:
    vkFreeCommandBuffers(vk->device, vk->gpu.single_use_command_pool, 1, &cmd);
    if (fence) vkDestroyFence(vk->device, fence, NULL);
}

static bool
vulkano_resized(struct vulkano* vk, VulkanoError* error)
{
    VkSurfaceCapabilitiesKHR capabilities = {0};
    VULKANO_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            vk->gpu.handle, vk->surface, &capabilities
        ),
        error
    );
    if (*error) return true;

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFF) {
        uint32_t width, height;
        vk->query_size(&width, &height);
        extent.width = VULKANO_CLAMP(
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width,
            width
        );
        extent.height = VULKANO_CLAMP(
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height,
            height
        );
    }
    return (
        extent.height != vk->swapchain.extent.height ||
        extent.width != vk->swapchain.extent.width
    );
}

void
vulkano_frame_acquire(
    struct vulkano* vk, struct vulkano_frame* frame, VulkanoError* error
)
{
    assert(
        vk->swapchain.image_count &&
        "you probably need to call vulkano_configure_swapchain"
    );
    if (*error) return;

    if (vulkano_resized(vk, error)) {
        destroy_swapchain(vk);
        create_swapchain(vk, error);
        if (*error) return;
    }
    if (vk->swapchain.handle == VK_NULL_HANDLE) {
        create_swapchain(vk, error);
        if (*error) return;
    }

    frame->number = vk->frame_counter++;
    frame->index  = frame->number % vk->swapchain.image_count;
    frame->state  = vk->frame_state[frame->index];

    VULKANO_CHECK(
        vkWaitForFences(
            vk->device,
            1,
            &frame->state.presentation_complete,
            VK_TRUE,
            VULKANO_TIMEOUT
        ),
        error
    );
    VULKANO_CHECK(
        vkResetFences(vk->device, 1, &frame->state.presentation_complete), error
    );
    VULKANO_CHECK(vkResetCommandBuffer(frame->state.render_command, 0), error);
    if (*error) return;

acquire_image : {
    VkResult result = vkAcquireNextImageKHR(
        vk->device,
        vk->swapchain.handle,
        VULKANO_TIMEOUT,
        frame->state.image_ready_for_use,
        VK_NULL_HANDLE,
        &frame->image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        destroy_swapchain(vk);
        create_swapchain(vk, error);
        if (*error) return;
        goto acquire_image;
    }
    if (result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT) {
        // FIXME: handle this case
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("full screen exclusive mode lost not implemented\n");
        return;
    }
    if (result != VK_SUCCESS) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        return;
    }

    frame->framebuffer = vk->swapchain.framebuffers[frame->image_index];
    VkCommandBufferBeginInfo command_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    VULKANO_CHECK(
        vkBeginCommandBuffer(frame->state.render_command, &command_begin_info),
        error
    );
    if (*error) return;
}

    // TODO: add configurable depth clear value to frame
    VkClearValue clear_value[] = {
        {
            .color.float32 =
                {
                    frame->clear[0],
                    frame->clear[1],
                    frame->clear[2],
                    frame->clear[3],
                },
        },
        {
            .depthStencil =
                {
                    .depth = 1.0f,
                },
        },
    };
    VkRenderPassBeginInfo render_begin_info = {
        .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass        = vk->swapchain.render_pass,
        .framebuffer       = frame->framebuffer,
        .renderArea.extent = vk->swapchain.extent,
        .clearValueCount   = 2,
        .pClearValues      = clear_value,
    };
    vkCmdBeginRenderPass(
        frame->state.render_command,
        &render_begin_info,
        VK_SUBPASS_CONTENTS_INLINE
    );
}

void
vulkano_frame_submit(
    struct vulkano*       vk,
    struct vulkano_frame* frame,
    VkSubmitInfo          info,
    VulkanoError*         error
)
{
    if (*error) return;

    vkCmdEndRenderPass(frame->state.render_command);

    VULKANO_CHECK(vkEndCommandBuffer(frame->state.render_command), error);
    if (*error) return;

    // add library wait semaphores to user provided ones
    VkPipelineStageFlags  wait_mask[32];
    VkSemaphore           wait_semaphores[32];
    static const uint32_t library_wait_count = 1;
    uint32_t              total_wait_semaphores =
        library_wait_count + info.waitSemaphoreCount;
    assert(total_wait_semaphores <= 32);

    wait_mask[0]       = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    wait_semaphores[0] = frame->state.image_ready_for_use;

    for (uint32_t i = 0; i < total_wait_semaphores - library_wait_count; i++) {
        wait_mask[library_wait_count + i]       = info.pWaitDstStageMask[i];
        wait_semaphores[library_wait_count + i] = info.pWaitSemaphores[i];
    }

    // add library signal semaphores to user provided ones
    VkSemaphore           signal_semaphores[32];
    static const uint32_t library_signal_count = 1;
    uint32_t              total_signal_semaphores =
        library_signal_count + info.signalSemaphoreCount;
    assert(total_signal_semaphores <= 32);

    signal_semaphores[0] = frame->state.rendering_commands_complete;

    for (uint32_t i = 0; i < total_signal_semaphores - library_signal_count;
         i++) {
        signal_semaphores[library_signal_count + i] = info.pSignalSemaphores[i];
    }

    assert(
        info.commandBufferCount == 0 &&
        "submitting additional commands alongside render command not supported"
    );

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = total_wait_semaphores,
        .pWaitSemaphores      = wait_semaphores,
        .pWaitDstStageMask    = wait_mask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &frame->state.render_command,
        .signalSemaphoreCount = total_signal_semaphores,
        .pSignalSemaphores    = signal_semaphores,
    };
    VULKANO_CHECK(
        vkQueueSubmit(
            vk->gpu.graphics_queue,
            1,
            &submit_info,
            frame->state.presentation_complete
        ),
        error
    );
    if (*error) return;

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame->state.rendering_commands_complete,
        .swapchainCount     = 1,
        .pSwapchains        = &vk->swapchain.handle,
        .pImageIndices      = &frame->image_index,
    };
    VkResult result = vkQueuePresentKHR(vk->gpu.graphics_queue, &present_info);
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        return;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // next frame will recreate the swapchain,
        return;
    }
    if (result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT) {
        // FIXME: handle this case
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("full screen exclusive mode lost not implemented\n");
        return;
    }
    *error = VULKANO_ERROR_CODE_FATAL_ERROR;
}

static uint32_t
select_memory_type(
    struct vulkano_gpu    gpu,
    uint32_t              required_memory_type_bits,
    VkMemoryPropertyFlags required_memory_property_flags,
    VulkanoError*         error
)
{
    if (*error) return 0;

    for (uint32_t i = 0; i < gpu.memory_properties.memoryTypeCount; i++) {
        uint32_t memory_type_check = (1 << i) & required_memory_type_bits;
        uint32_t memory_properties_check =
            required_memory_property_flags ==
            (gpu.memory_properties.memoryTypes[i].propertyFlags &
             required_memory_property_flags);

        if (memory_type_check && memory_properties_check) {
            return i;
        }
    }

    *error = VULKANO_ERROR_CODE_FATAL_ERROR;
    VULKANO_ERROR("unable to find memory fitting the given requriements");
    return 0;
}

struct vulkano_buffer
vulkano_buffer_create(
    struct vulkano*       vk,
    VkBufferCreateInfo    info,
    VkMemoryPropertyFlags memory_properties,
    VulkanoError*         error
)
{
    struct vulkano_buffer buffer = {0};
    if (*error) return buffer;

    buffer.usage    = info.usage;
    buffer.capacity = info.size;
    DEFAULT0(info.sharingMode, VK_SHARING_MODE_EXCLUSIVE);
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    VULKANO_CHECK(
        vkCreateBuffer(vk->device, &info, NULL, &buffer.handle), error
    );
    if (*error) return buffer;

    VkMemoryRequirements memory_requirements = {0};
    vkGetBufferMemoryRequirements(
        vk->device, buffer.handle, &memory_requirements
    );
    uint32_t memory_type_index = select_memory_type(
        vk->gpu, memory_requirements.memoryTypeBits, memory_properties, error
    );
    if (*error) {
        vkDestroyBuffer(vk->device, buffer.handle, NULL);
        return buffer;
    }

    buffer.memory_flags =
        vk->gpu.memory_properties.memoryTypes[memory_type_index].propertyFlags;
    VkMemoryAllocateInfo allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };
    VULKANO_CHECK(
        vkAllocateMemory(vk->device, &allocate_info, NULL, &buffer.memory),
        error
    );
    if (*error) {
        vkDestroyBuffer(vk->device, buffer.handle, NULL);
        return buffer;
    }
    VULKANO_CHECK(
        vkBindBufferMemory(vk->device, buffer.handle, buffer.memory, 0), error
    );
    if (*error) {
        vkDestroyBuffer(vk->device, buffer.handle, NULL);
        vkFreeMemory(vk->device, buffer.memory, NULL);
        return buffer;
    }

    return buffer;
}

void
vulkano_buffer_destroy(struct vulkano* vk, struct vulkano_buffer* buffer)
{
    if (!buffer) return;
    if (buffer->handle) vkDestroyBuffer(vk->device, buffer->handle, NULL);
    if (buffer->memory) vkFreeMemory(vk->device, buffer->memory, NULL);
    *buffer = (struct vulkano_buffer){0};
}

static void
vulkano_buffer_copy_to_host_coherent(
    struct vulkano*        vk,
    struct vulkano_buffer* buffer,
    struct vulkano_data    data,
    VulkanoError*          error
)
{
    if (*error) return;

    void* memory = NULL;
    VULKANO_CHECK(
        vkMapMemory(vk->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &memory),
        error
    );
    if (*error) return;
    memcpy(memory, data.data, data.size);
    vkUnmapMemory(vk->device, buffer->memory);
}

static void
vulkano_buffer_copy_to_host_visible(
    struct vulkano*        vk,
    struct vulkano_buffer* buffer,
    struct vulkano_data    data,
    VulkanoError*          error
)
{
    if (*error) return;

    void* memory = NULL;
    VULKANO_CHECK(
        vkMapMemory(vk->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &memory),
        error
    );
    if (*error) return;
    memcpy(memory, data.data, data.size);

    VkMappedMemoryRange range = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer->memory,
        .offset = 0,
        .size   = VK_WHOLE_SIZE,
    };
    VULKANO_CHECK(vkFlushMappedMemoryRanges(vk->device, 1, &range), error);
    vkUnmapMemory(vk->device, buffer->memory);
    if (*error) return;
}

static void
vulkano_buffer_copy_to_device_local(
    struct vulkano*        vk,
    struct vulkano_buffer* buffer,
    struct vulkano_data    data,
    VulkanoError*          error
)
{
    if (*error) return;

    if (!(buffer->usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(
            "trying to copy to a buffer without "
            "VK_BUFFER_USAGE_TRANSFER_DST_BIT in "
            "usage"
        );
        return;
    }


    struct vulkano_buffer transfer_buffer = vulkano_buffer_create(
        vk,
        (VkBufferCreateInfo){
            .size  = data.size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        },
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        error
    );
    vulkano_buffer_copy_to(vk, &transfer_buffer, data, error);
    VkCommandBuffer cmd = vulkano_acquire_single_use_command_buffer(vk, error);
    if (*error) goto cleanup;

    vkCmdCopyBuffer(
        cmd,
        transfer_buffer.handle,
        buffer->handle,
        1,
        (VkBufferCopy[]){{.size = data.size}}
    );
    vulkano_submit_single_use_command_buffer(vk, cmd, error);

cleanup:
    vulkano_buffer_destroy(vk, &transfer_buffer);
}

void
vulkano_buffer_copy_to(
    struct vulkano*        vk,
    struct vulkano_buffer* buffer,
    struct vulkano_data    data,
    VulkanoError*          error
)
{
    if (*error) return;

    if (buffer->capacity < data.size) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("overflowing copy operation requested");
        return;
    }

    if (buffer->memory_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        vulkano_buffer_copy_to_host_coherent(vk, buffer, data, error);
    else if (buffer->memory_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        vulkano_buffer_copy_to_host_visible(vk, buffer, data, error);
    else
        vulkano_buffer_copy_to_device_local(vk, buffer, data, error);
}

struct vulkano_image
vulkano_image_create(
    struct vulkano*       vk,
    VkImageCreateInfo     info,
    VkMemoryPropertyFlags memory_flags,
    VulkanoError*         error
)
{
    struct vulkano_image image = {.layout = info.initialLayout};
    if (*error) return image;

    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    DEFAULT0(info.imageType, VK_IMAGE_TYPE_2D);
    DEFAULT0(info.extent.depth, 1);
    DEFAULT0(info.mipLevels, 1);
    DEFAULT0(info.arrayLayers, 1);
    DEFAULT0(info.format, vk->gpu.configured_surface_format.format);
    DEFAULT0(info.samples, VK_SAMPLE_COUNT_1_BIT);

    VULKANO_CHECK(vkCreateImage(vk->device, &info, NULL, &image.handle), error);
    if (*error) return image;

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(vk->device, image.handle, &requirements);
    uint32_t memory_type = select_memory_type(
        vk->gpu, requirements.memoryTypeBits, memory_flags, error
    );
    if (*error) {
        vulkano_image_destroy(vk, &image);
        return image;
    }

    VkMemoryAllocateInfo allocation_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memory_type,
    };
    VULKANO_CHECK(
        vkAllocateMemory(vk->device, &allocation_info, NULL, &image.memory),
        error
    );
    VULKANO_CHECK(
        vkBindImageMemory(vk->device, image.handle, image.memory, 0), error
    );
    if (*error) {
        vulkano_image_destroy(vk, &image);
        return image;
    }

    return image;
}

void
vulkano_image_destroy(struct vulkano* vk, struct vulkano_image* image)
{
    if (!image) return;
    if (image->memory) vkFreeMemory(vk->device, image->memory, NULL);
    if (image->handle) vkDestroyImage(vk->device, image->handle, NULL);
    *image = (struct vulkano_image){0};
}

void
vulkano_image_change_layout(
    struct vulkano*       vk,
    struct vulkano_image* image,
    VkImageLayout         layout,
    VulkanoError*         error
)
{
    if (*error) return;

    VkImageMemoryBarrier barrier = {
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout                   = image->layout,
        .newLayout                   = layout,
        .srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
        .image                       = image->handle,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };

    VkPipelineStageFlags dest_stage_flags;
    VkPipelineStageFlags source_stage_flags;

    if (image->layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage_flags    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage_flags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (image->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage_flags    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dest_stage_flags      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR("unimplemented layout transition");
        return;
    }

    image->layout = layout;

    VkCommandBuffer cmd = vulkano_acquire_single_use_command_buffer(vk, error);
    if (*error) return;

    vkCmdPipelineBarrier(
        cmd,
        source_stage_flags,
        dest_stage_flags,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier
    );
    vulkano_submit_single_use_command_buffer(vk, cmd, error);
    if (*error) return;
}

void
vulkano_image_copy_to(
    struct vulkano*           vk,
    struct vulkano_image*     image,
    struct vulkano_image_data data,
    VulkanoError*             error
)
{
    if (*error) return;

    VkBufferImageCopy copy_info = {
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.layerCount = 1,
        .imageExtent                 = {data.width, data.height, 1},
    };

    struct vulkano_buffer transfer_buffer = vulkano_buffer_create(
        vk,
        (VkBufferCreateInfo){
            .size  = data.size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        },
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        error
    );
    vulkano_buffer_copy_to(
        vk,
        &transfer_buffer,
        (struct vulkano_data){.size = data.size, .data = data.pixels},
        error
    );

    VkCommandBuffer cmd = vulkano_acquire_single_use_command_buffer(vk, error);
    if (*error) goto cleanup;

    vkCmdCopyBufferToImage(
        cmd, transfer_buffer.handle, image->handle, image->layout, 1, &copy_info
    );
    vulkano_submit_single_use_command_buffer(vk, cmd, error);

cleanup:
    vulkano_buffer_destroy(vk, &transfer_buffer);
}

struct vulkano_sampler
vulkano_sampler_create(
    struct vulkano* vk, struct vulkano_image_data data, VulkanoError* error
)
{
    struct vulkano_sampler sampler = {0};
    if (*error) return sampler;

    sampler.image = vulkano_image_create(
        vk,
        (VkImageCreateInfo){
            .format = data.format,
            .extent =
                {
                    .width  = data.width,
                    .height = data.height,
                    .depth  = 1,
                },
            .usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        error
    );
    vulkano_image_change_layout(
        vk, &sampler.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, error
    );
    vulkano_image_copy_to(vk, &sampler.image, data, error);
    vulkano_image_change_layout(
        vk, &sampler.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, error
    );
    sampler.view = vulkano_create_image_view(
        vk,
        (VkImageViewCreateInfo){
            .image  = sampler.image.handle,
            .format = data.format,
        },
        error
    );
    sampler.handle = vulkano_create_sampler(
        vk,
        (VkSamplerCreateInfo){
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy    = vk->gpu.properties.limits.maxSamplerAnisotropy,
        },
        error
    );
    if (*error) vulkano_sampler_destroy(vk, &sampler);
    return sampler;
}

void
vulkano_sampler_destroy(struct vulkano* vk, struct vulkano_sampler* sampler)
{
    if (vk->device) vkDeviceWaitIdle(vk->device);
    vulkano_image_destroy(vk, &sampler->image);
    vkDestroyImageView(vk->device, sampler->view, NULL);
    vkDestroySampler(vk->device, sampler->handle, NULL);
    *sampler = (struct vulkano_sampler){0};
}

static uint64_t
score_gpu(VkPhysicalDevice gpu)
{
    uint64_t score;

    // score gpus in tiers where Discrete > Integrated > Virtual reguardless of
    // other properties
    {
        static const uint64_t DISCRETE   = (uint64_t)1 << 63;
        static const uint64_t INTEGRATED = (uint64_t)1 << 62;
        static const uint64_t VIRTUAL    = (uint64_t)1 << 61;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score = DISCRETE;
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            score = INTEGRATED;
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            score = VIRTUAL;
        else
            score = 0;
    }

    // score gpus within the same tier based on vram
    {
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

        for (size_t i = 0; i < memory_properties.memoryHeapCount; i++) {
            VkMemoryHeap heap = memory_properties.memoryHeaps[i];
            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                score += heap.size;
                break;
            }
        }
    }

    return score;
}

int
default_gpu_compare(VkPhysicalDevice* gpu1, VkPhysicalDevice* gpu2)
{
    uint64_t score1 = score_gpu(*gpu1);
    uint64_t score2 = score_gpu(*gpu2);
    if (score1 == score2) return 0;
    if (score1 < score2) return -1;
    return 1;
}

static uint32_t
score_present_mode(VkPresentModeKHR mode)
{
    switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR: return 1;
        case VK_PRESENT_MODE_MAILBOX_KHR: return 4;
        case VK_PRESENT_MODE_FIFO_KHR: return 3;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return 2;
        default: return 0;
    }
}

int
default_present_modes_compare(VkPresentModeKHR* mode1, VkPresentModeKHR* mode2)
{
    uint32_t score1 = score_present_mode(*mode1);
    uint32_t score2 = score_present_mode(*mode2);
    if (score1 > score2) return 1;
    if (score1 == score2) return 0;
    return -1;
}

static uint32_t
score_surface_format(VkSurfaceFormatKHR format)
{
    uint32_t STANDARD_COLOR_SPACE_BIT  = (uint32_t)1 << 30;
    uint32_t STANDARD_COLOR_FORMAT_BIT = (uint32_t)1 << 29;

    uint32_t score = 0;
    if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        score &= STANDARD_COLOR_SPACE_BIT;
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
        score &= STANDARD_COLOR_FORMAT_BIT;

    return score;
}

int
default_surface_format_compare(
    VkSurfaceFormatKHR* fmt1, VkSurfaceFormatKHR* fmt2
)
{
    uint32_t score1 = score_surface_format(*fmt1);
    uint32_t score2 = score_surface_format(*fmt2);
    if (score1 > score2) return 1;
    if (score1 == score2) return 0;
    return -1;
}

static int
compare_layer_properties_name(
    VkLayerProperties* prop1, VkLayerProperties* prop2
)
{
    return strcmp(prop1->layerName, prop2->layerName);
}

static int
compare_extension_names(
    const VkExtensionProperties* ext1, const VkExtensionProperties* ext2
)
{
    return strcmp(ext1->extensionName, ext2->extensionName);
}

// only for use during initialization
static struct string_array
combine_string_arrays_unique(
    struct string_array array1, struct string_array array2
)
{
    uint32_t total_count = array1.count + array2.count;
    if (total_count == 0)
        return (struct string_array){.count = 0, .data = NULL};

    struct string_array combined = {
        .data =
            init_malloc((array1.count + array2.count) * sizeof(const char*)),
    };
    if (array1.data) {
        combined.count = array1.count;
        memcpy(
            combined.data, array1.data, sizeof(array1.data[0]) * array1.count
        );
        qsort(
            array1.data,
            array1.count,
            sizeof(const char*),
            (compare_function)strcmp
        );
    }
    if (array2.data)
        for (uint32_t i = 0; i < array2.count; i++) {
            if (!array1.data || !bsearch(
                                    array2.data[i],
                                    array1.data,
                                    array1.count,
                                    sizeof(array1.data[0]),
                                    (compare_function)strcmp
                                ))
                memcpy(
                    combined.data + combined.count++,
                    array2.data + i,
                    sizeof(array1.data[0])
                );
        }
    return combined;
}

static const char*
gpu_name(VkPhysicalDevice gpu)
{
    static char                buffer[256];
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpu, &properties);
    snprintf(buffer, sizeof(buffer), "%s", properties.deviceName);
    return buffer;
}

static void*
init_malloc(size_t size)
{
    if (init_allocations.count == init_allocations.capacity) {
        init_allocations.capacity    = 1 + (init_allocations.capacity * 2);
        init_allocations.allocations = realloc(
            init_allocations.allocations,
            (sizeof *init_allocations.allocations) * init_allocations.capacity
        );
        if (!init_allocations.allocations) goto memory_error;
    }
    void* allocation = malloc(size);
    if (!allocation) goto memory_error;
    init_allocations.allocations[init_allocations.count++] = allocation;
    return allocation;

memory_error:
    VULKANO_ERROR("ran out of memory during initialization");
    exit(EXIT_FAILURE);
}

static void
free_init_allocations(void)
{
    for (size_t i = 0; i < init_allocations.count; i++) {
        free(init_allocations.allocations[i]);
    }
    free(init_allocations.allocations);
    memset(&init_allocations, 0, sizeof(init_allocations));
}

const char*
vkresult_to_string(VkResult result)
{
    switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#endif
        case VK_RESULT_MAX_ENUM: return "VK_RESULT_MAX_ENUM";
    }

    return "Unrecognized VkResult";
}

const char*
present_mode_to_string(VkPresentModeKHR mode)
{
    switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR: return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR: return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
        case VK_PRESENT_MODE_MAX_ENUM_KHR:
            return "VK_PRESENT_MODE_MAX_ENUM_KHR";
    }
    return "Unrecognized present mode";
}

const char*
color_format_to_string(VkFormat fmt)
{
    switch (fmt) {
        case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
        case VK_FORMAT_R4G4_UNORM_PACK8: return "VK_FORMAT_R4G4_UNORM_PACK8";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case VK_FORMAT_R8_UNORM: return "VK_FORMAT_R8_UNORM";
        case VK_FORMAT_R8_SNORM: return "VK_FORMAT_R8_SNORM";
        case VK_FORMAT_R8_USCALED: return "VK_FORMAT_R8_USCALED";
        case VK_FORMAT_R8_SSCALED: return "VK_FORMAT_R8_SSCALED";
        case VK_FORMAT_R8_UINT: return "VK_FORMAT_R8_UINT";
        case VK_FORMAT_R8_SINT: return "VK_FORMAT_R8_SINT";
        case VK_FORMAT_R8_SRGB: return "VK_FORMAT_R8_SRGB";
        case VK_FORMAT_R8G8_UNORM: return "VK_FORMAT_R8G8_UNORM";
        case VK_FORMAT_R8G8_SNORM: return "VK_FORMAT_R8G8_SNORM";
        case VK_FORMAT_R8G8_USCALED: return "VK_FORMAT_R8G8_USCALED";
        case VK_FORMAT_R8G8_SSCALED: return "VK_FORMAT_R8G8_SSCALED";
        case VK_FORMAT_R8G8_UINT: return "VK_FORMAT_R8G8_UINT";
        case VK_FORMAT_R8G8_SINT: return "VK_FORMAT_R8G8_SINT";
        case VK_FORMAT_R8G8_SRGB: return "VK_FORMAT_R8G8_SRGB";
        case VK_FORMAT_R8G8B8_UNORM: return "VK_FORMAT_R8G8B8_UNORM";
        case VK_FORMAT_R8G8B8_SNORM: return "VK_FORMAT_R8G8B8_SNORM";
        case VK_FORMAT_R8G8B8_USCALED: return "VK_FORMAT_R8G8B8_USCALED";
        case VK_FORMAT_R8G8B8_SSCALED: return "VK_FORMAT_R8G8B8_SSCALED";
        case VK_FORMAT_R8G8B8_UINT: return "VK_FORMAT_R8G8B8_UINT";
        case VK_FORMAT_R8G8B8_SINT: return "VK_FORMAT_R8G8B8_SINT";
        case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
        case VK_FORMAT_B8G8R8_UNORM: return "VK_FORMAT_B8G8R8_UNORM";
        case VK_FORMAT_B8G8R8_SNORM: return "VK_FORMAT_B8G8R8_SNORM";
        case VK_FORMAT_B8G8R8_USCALED: return "VK_FORMAT_B8G8R8_USCALED";
        case VK_FORMAT_B8G8R8_SSCALED: return "VK_FORMAT_B8G8R8_SSCALED";
        case VK_FORMAT_B8G8R8_UINT: return "VK_FORMAT_B8G8R8_UINT";
        case VK_FORMAT_B8G8R8_SINT: return "VK_FORMAT_B8G8R8_SINT";
        case VK_FORMAT_B8G8R8_SRGB: return "VK_FORMAT_B8G8R8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_USCALED: return "VK_FORMAT_R8G8B8A8_USCALED";
        case VK_FORMAT_R8G8B8A8_SSCALED: return "VK_FORMAT_R8G8B8A8_SSCALED";
        case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_USCALED: return "VK_FORMAT_B8G8R8A8_USCALED";
        case VK_FORMAT_B8G8R8A8_SSCALED: return "VK_FORMAT_B8G8R8A8_SSCALED";
        case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case VK_FORMAT_R16_UNORM: return "VK_FORMAT_R16_UNORM";
        case VK_FORMAT_R16_SNORM: return "VK_FORMAT_R16_SNORM";
        case VK_FORMAT_R16_USCALED: return "VK_FORMAT_R16_USCALED";
        case VK_FORMAT_R16_SSCALED: return "VK_FORMAT_R16_SSCALED";
        case VK_FORMAT_R16_UINT: return "VK_FORMAT_R16_UINT";
        case VK_FORMAT_R16_SINT: return "VK_FORMAT_R16_SINT";
        case VK_FORMAT_R16_SFLOAT: return "VK_FORMAT_R16_SFLOAT";
        case VK_FORMAT_R16G16_UNORM: return "VK_FORMAT_R16G16_UNORM";
        case VK_FORMAT_R16G16_SNORM: return "VK_FORMAT_R16G16_SNORM";
        case VK_FORMAT_R16G16_USCALED: return "VK_FORMAT_R16G16_USCALED";
        case VK_FORMAT_R16G16_SSCALED: return "VK_FORMAT_R16G16_SSCALED";
        case VK_FORMAT_R16G16_UINT: return "VK_FORMAT_R16G16_UINT";
        case VK_FORMAT_R16G16_SINT: return "VK_FORMAT_R16G16_SINT";
        case VK_FORMAT_R16G16_SFLOAT: return "VK_FORMAT_R16G16_SFLOAT";
        case VK_FORMAT_R16G16B16_UNORM: return "VK_FORMAT_R16G16B16_UNORM";
        case VK_FORMAT_R16G16B16_SNORM: return "VK_FORMAT_R16G16B16_SNORM";
        case VK_FORMAT_R16G16B16_USCALED: return "VK_FORMAT_R16G16B16_USCALED";
        case VK_FORMAT_R16G16B16_SSCALED: return "VK_FORMAT_R16G16B16_SSCALED";
        case VK_FORMAT_R16G16B16_UINT: return "VK_FORMAT_R16G16B16_UINT";
        case VK_FORMAT_R16G16B16_SINT: return "VK_FORMAT_R16G16B16_SINT";
        case VK_FORMAT_R16G16B16_SFLOAT: return "VK_FORMAT_R16G16B16_SFLOAT";
        case VK_FORMAT_R16G16B16A16_UNORM:
            return "VK_FORMAT_R16G16B16A16_UNORM";
        case VK_FORMAT_R16G16B16A16_SNORM:
            return "VK_FORMAT_R16G16B16A16_SNORM";
        case VK_FORMAT_R16G16B16A16_USCALED:
            return "VK_FORMAT_R16G16B16A16_USCALED";
        case VK_FORMAT_R16G16B16A16_SSCALED:
            return "VK_FORMAT_R16G16B16A16_SSCALED";
        case VK_FORMAT_R16G16B16A16_UINT: return "VK_FORMAT_R16G16B16A16_UINT";
        case VK_FORMAT_R16G16B16A16_SINT: return "VK_FORMAT_R16G16B16A16_SINT";
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case VK_FORMAT_R32_UINT: return "VK_FORMAT_R32_UINT";
        case VK_FORMAT_R32_SINT: return "VK_FORMAT_R32_SINT";
        case VK_FORMAT_R32_SFLOAT: return "VK_FORMAT_R32_SFLOAT";
        case VK_FORMAT_R32G32_UINT: return "VK_FORMAT_R32G32_UINT";
        case VK_FORMAT_R32G32_SINT: return "VK_FORMAT_R32G32_SINT";
        case VK_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
        case VK_FORMAT_R32G32B32_UINT: return "VK_FORMAT_R32G32B32_UINT";
        case VK_FORMAT_R32G32B32_SINT: return "VK_FORMAT_R32G32B32_SINT";
        case VK_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
        case VK_FORMAT_R32G32B32A32_UINT: return "VK_FORMAT_R32G32B32A32_UINT";
        case VK_FORMAT_R32G32B32A32_SINT: return "VK_FORMAT_R32G32B32A32_SINT";
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case VK_FORMAT_R64_UINT: return "VK_FORMAT_R64_UINT";
        case VK_FORMAT_R64_SINT: return "VK_FORMAT_R64_SINT";
        case VK_FORMAT_R64_SFLOAT: return "VK_FORMAT_R64_SFLOAT";
        case VK_FORMAT_R64G64_UINT: return "VK_FORMAT_R64G64_UINT";
        case VK_FORMAT_R64G64_SINT: return "VK_FORMAT_R64G64_SINT";
        case VK_FORMAT_R64G64_SFLOAT: return "VK_FORMAT_R64G64_SFLOAT";
        case VK_FORMAT_R64G64B64_UINT: return "VK_FORMAT_R64G64B64_UINT";
        case VK_FORMAT_R64G64B64_SINT: return "VK_FORMAT_R64G64B64_SINT";
        case VK_FORMAT_R64G64B64_SFLOAT: return "VK_FORMAT_R64G64B64_SFLOAT";
        case VK_FORMAT_R64G64B64A64_UINT: return "VK_FORMAT_R64G64B64A64_UINT";
        case VK_FORMAT_R64G64B64A64_SINT: return "VK_FORMAT_R64G64B64A64_SINT";
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case VK_FORMAT_D16_UNORM: return "VK_FORMAT_D16_UNORM";
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return "VK_FORMAT_X8_D24_UNORM_PACK32";
        case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
        case VK_FORMAT_S8_UINT: return "VK_FORMAT_S8_UINT";
        case VK_FORMAT_D16_UNORM_S8_UINT: return "VK_FORMAT_D16_UNORM_S8_UINT";
        case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case VK_FORMAT_BC2_UNORM_BLOCK: return "VK_FORMAT_BC2_UNORM_BLOCK";
        case VK_FORMAT_BC2_SRGB_BLOCK: return "VK_FORMAT_BC2_SRGB_BLOCK";
        case VK_FORMAT_BC3_UNORM_BLOCK: return "VK_FORMAT_BC3_UNORM_BLOCK";
        case VK_FORMAT_BC3_SRGB_BLOCK: return "VK_FORMAT_BC3_SRGB_BLOCK";
        case VK_FORMAT_BC4_UNORM_BLOCK: return "VK_FORMAT_BC4_UNORM_BLOCK";
        case VK_FORMAT_BC4_SNORM_BLOCK: return "VK_FORMAT_BC4_SNORM_BLOCK";
        case VK_FORMAT_BC5_UNORM_BLOCK: return "VK_FORMAT_BC5_UNORM_BLOCK";
        case VK_FORMAT_BC5_SNORM_BLOCK: return "VK_FORMAT_BC5_SNORM_BLOCK";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case VK_FORMAT_BC7_UNORM_BLOCK: return "VK_FORMAT_BC7_UNORM_BLOCK";
        case VK_FORMAT_BC7_SRGB_BLOCK: return "VK_FORMAT_BC7_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        case VK_FORMAT_G8B8G8R8_422_UNORM:
            return "VK_FORMAT_G8B8G8R8_422_UNORM";
        case VK_FORMAT_B8G8R8G8_422_UNORM:
            return "VK_FORMAT_B8G8R8G8_422_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
            return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
        case VK_FORMAT_R10X6_UNORM_PACK16:
            return "VK_FORMAT_R10X6_UNORM_PACK16";
        case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
            return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
        case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
        case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
        case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_R12X4_UNORM_PACK16:
            return "VK_FORMAT_R12X4_UNORM_PACK16";
        case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
            return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
        case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
        case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
        case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_G16B16G16R16_422_UNORM:
            return "VK_FORMAT_G16B16G16R16_422_UNORM";
        case VK_FORMAT_B16G16R16G16_422_UNORM:
            return "VK_FORMAT_B16G16R16G16_422_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
            return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
            return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
        case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
            return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
            return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
            return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
        case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
            return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
            return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
            return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
        case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
        case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
            return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_R16G16_S10_5_NV: return "VK_FORMAT_R16G16_S10_5_NV";
        case VK_FORMAT_MAX_ENUM: return "VK_FORMAT_MAX_ENUM";
    }
    return "Unrecognized VkFormat";
}

const char*
color_space_to_string(VkColorSpaceKHR space)
{
    switch (space) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
        case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            return "VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_BT709_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            return "VK_COLOR_SPACE_BT2020_LINEAR_EXT";
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            return "VK_COLOR_SPACE_HDR10_ST2084_EXT";
        case VK_COLOR_SPACE_DOLBYVISION_EXT:
            return "VK_COLOR_SPACE_DOLBYVISION_EXT";
        case VK_COLOR_SPACE_HDR10_HLG_EXT:
            return "VK_COLOR_SPACE_HDR10_HLG_EXT";
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            return "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT";
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
        case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            return "VK_COLOR_SPACE_PASS_THROUGH_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT";
        case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
            return "VK_COLOR_SPACE_DISPLAY_NATIVE_AMD";
        case VK_COLOR_SPACE_MAX_ENUM_KHR: return "VK_COLOR_SPACE_MAX_ENUM_KHR";
    }
    return "Unrecognized VkColorSpaceKHR";
}

#ifdef VULKANO_INTEGRATE_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

SDL_Window* vulkano_sdl_window;

const char*
create_surface(VkInstance instance, VkSurfaceKHR* surface)
{
    if (!SDL_Vulkan_CreateSurface(vulkano_sdl_window, instance, surface)) {
        return SDL_GetError();
    }
    return NULL;
}

void
query_size(uint32_t* width, uint32_t* height)
{
    int w, h;
    SDL_Vulkan_GetDrawableSize(vulkano_sdl_window, &w, &h);
    w       = (w < 0) ? 720 : w;
    h       = (h < 0) ? 480 : h;
    *width  = (uint32_t)w;
    *height = (uint32_t)h;
}

struct vulkano
vulkano_from_sdl(
    struct vulkano_config vkcfg, struct SDL_Window* window, VulkanoError* error
)
{
    vulkano_sdl_window      = window;
    vkcfg.surface_creation  = create_surface;
    vkcfg.query_window_size = query_size;

    struct vulkano vk = {0};
    if (*error) return vk;

    // enumerate sdl required instance extensions and combine them with user
    // required instance extensions
    struct string_array user_instance_extensions = {
        .count = vkcfg.instance_extensions_count,
        .data  = vkcfg.instance_extensions,
    };
    struct string_array sdl_required_extensions      = {0};
    struct string_array combined_instance_extensions = {0};

    if (!SDL_Vulkan_GetInstanceExtensions(
            window, &sdl_required_extensions.count, NULL
        )) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(SDL_GetError());
        return vk;
    }
    const char** extensions;
    VULKANO_INIT_MALLOC_ARRAY(extensions, sdl_required_extensions.count);
    sdl_required_extensions.data = extensions;
    if (!SDL_Vulkan_GetInstanceExtensions(
            window, &sdl_required_extensions.count, sdl_required_extensions.data
        )) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(SDL_GetError());
        goto cleanup;
    }

    combined_instance_extensions = combine_string_arrays_unique(
        user_instance_extensions, sdl_required_extensions
    );
    if (*error) goto cleanup;
    vkcfg.instance_extensions       = combined_instance_extensions.data;
    vkcfg.instance_extensions_count = combined_instance_extensions.count;

    // initialize vulkano
    vk = vulkano_create(vkcfg, error);
    if (*error) goto cleanup;

cleanup:
    free_init_allocations();
    if (*error) vulkano_destroy(&vk);
    return vk;
}

struct vulkano_sdl
vulkano_sdl_create(
    struct vulkano_config vkcfg, struct sdl_config sdlcfg, VulkanoError* error
)
{
    struct vulkano_sdl vksdl = {0};
    if (*error) return vksdl;

    DEFAULT0(sdlcfg.title, "vulkano sdl window");
    DEFAULT0(sdlcfg.width, 720);
    DEFAULT0(sdlcfg.height, 480);

    // initialize sdl
    if (SDL_Init(sdlcfg.init_flags | SDL_INIT_VIDEO)) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(SDL_GetError());
        return vksdl;
    }
    vulkano_sdl_window = SDL_CreateWindow(
        sdlcfg.title,
        sdlcfg.left,
        sdlcfg.top,
        sdlcfg.width,
        sdlcfg.height,
        sdlcfg.window_flags | SDL_WINDOW_VULKAN
    );
    if (!vulkano_sdl_window) {
        *error = VULKANO_ERROR_CODE_FATAL_ERROR;
        VULKANO_ERROR(SDL_GetError());
        SDL_Quit();
        return vksdl;
    }

    vksdl.sdl = vulkano_sdl_window;
    vksdl.vk  = vulkano_from_sdl(vkcfg, vulkano_sdl_window, error);
    return vksdl;
}

void
vulkano_sdl_destroy(struct vulkano_sdl* vksdl)
{
    vulkano_destroy(&vksdl->vk);
    if (vksdl->sdl) SDL_DestroyWindow(vksdl->sdl);
    SDL_Quit();
}

#endif  // VULKANO_INTEGRATE_SDL

#endif  // VULKANO_IMPLEMENTATION

#endif  // VULKANO_H
