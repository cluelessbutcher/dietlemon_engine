#pragma once

#include "vulkan_types.inl"

void vulkan_image_create(vulkan_context* context,
                         VkImageType image_type,
                         uint32_t width,
                         uint32_t height,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageUserFlags usage,
                         VkMemoryPropertyFlags memory_flags,
                         float create_view,
                         VkImageAspectFlags view_aspect_flags,
                         vulkan_image out_image);

void vulkan_image_view_create(vulkan_context* context, VkFormat format, vulkan_image* image, VkImageAspectFlags aspect_flags);
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);
