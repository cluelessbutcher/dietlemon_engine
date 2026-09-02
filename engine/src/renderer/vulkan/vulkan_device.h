#pragma once

#include "vulkan_types.inl"

bool vulkan_device_create(vulkan_context* context);
void vulkan_device_destroy(vulkan_context* context);
void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vulkan_swapchain_support_info* out_support_info); 
bool vulkan_device_detect_depth_format(vulkan_device* device);
