#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* out_swapchain);
void vulkan_swapchain_recreate(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* out_swapchain);
void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain);
bool vulkan_swapchain_acquire_next_image_index(vulkan_context* context, vulkan_swapchain* swapchain, uint64_t timeout_ms, VkSemaphore image_available_semaphore, VkFence fence, uint32_t* out_image_index);
void vulkan_swapchain_present(vulkan_context* context, vulkan_swapchain* swapchain, VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t present_image_index);
