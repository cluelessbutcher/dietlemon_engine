#pragma once

#include "defines.h"

#include "core/asserts.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                          \
    {                                           \
        DASSERT(expr == VK_SUCCESS);            \
    }                                           \

typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t format_count;
    VkSurfaceFormatKHR* formats;
    uint32_t present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    vulkan_swapchain_support_info swapchain_support;
    int32_t graphics_queue_index;
    int32_t present_queue_index;
    int32_t transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;
    
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    uint32_t width;
    uint32_t height;
} vulkan_image;

typedef enum vulkan_render_pass_state {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} vulkan_render_pass_state;

typedef struct vulkan_renderpass {
    VkRenderPass handle;
    float x, y, w, h;
    float r, g, b, a;
    float depth;
    uint32_t stencil;
    vulkan_render_pass_state state;
} vulkan_renderpass;

typedef struct vulkan_framebuffer {
    VkFramebuffer handle;
    uint32_t attachment_count;
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;
    uint8_t max_frames_in_flight;
    VkSwapchainKHR handle;
    uint32_t image_count;
    VkImage* images;
    VkImageView* views;
    vulkan_image depth_attachment;
    vulkan_framebuffer* framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence {
    VkFence handle;
    bool is_signaled;
} vulkan_fence;

typedef struct vulkan_context {
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;

    uint64_t framebuffer_size_generation;
    uint64_t framebuffer_size_last_generation;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    
#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    
    vulkan_device device;

    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;
    vulkan_command_buffer* graphics_command_buffers;

    VkSemaphore* image_available_semaphores;
    VkSemaphore* queue_complete_semaphores;
    uint32_t in_flight_fence_count;
    vulkan_fence* in_flight_fences;
    vulkan_fence** images_in_flight;
    
    uint32_t image_index;
    uint32_t current_frame;
    bool recreating_swapchain;
    int32_t (*find_memory_index)(uint32_t type_filter, uint32_t property_flags);
} vulkan_context;
