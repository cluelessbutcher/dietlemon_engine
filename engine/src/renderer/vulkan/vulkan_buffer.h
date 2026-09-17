#pragma once

#include "vulkan_types.inl"

bool vulkan_buffer_create(vulkan_context* context, uint64_t size, VkBufferUsageFlagBits usage, uint32_t memory_property_flag, bool bind_on_create, vulkan_buffer* out_buffer);
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer);
bool vulkan_buffer_resize(vulkan_context* context, uint64_t new_size, vulkan_buffer* buffer, VkQueue queue, VkCommandPool pool);
void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset);
void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset, uint64_t size, uint32_t flags);
void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer);
void vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset, uint64_t size, uint32_t flags, const void* data);
void vulkan_buffer_copy_to(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, uint64_t source_offset, VkBuffer dest, uint64_t dest_offset, uint64_t size);
