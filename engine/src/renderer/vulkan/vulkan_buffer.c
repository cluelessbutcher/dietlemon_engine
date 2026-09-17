#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/dmemory.h"

#include <string.h>

bool vulkan_buffer_create(vulkan_context* context, uint64_t size, VkBufferUsageFlagBits usage, uint32_t memory_property_flags, bool bind_on_create, vulkan_buffer* out_buffer) {
  memset(out_buffer, 0, sizeof(vulkan_buffer));
  out_buffer->total_size = size;
  out_buffer->usage = usage;
  out_buffer->memory_property_flags = memory_property_flags;
  
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &out_buffer->handle));
  
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(context->device.logical_device, out_buffer->handle, &requirements);
  out_buffer->memory_index = context->find_memory_index(requirements.memoryTypeBits, out_buffer->memory_property_flags);
  if (out_buffer->memory_index == -1) {
    DERROR("Unable to create vulkan buffer because the required memory type index was not found");
    return false;
  }
  
   VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
   allocate_info.allocationSize = requirements.size;
   allocate_info.memoryTypeIndex = (uint32_t)out_buffer->memory_index;

   VkResult result = vkAllocateMemory(context->device.logical_device, &allocate_info, context->allocator, &out_buffer->memory);
   if (result != VK_SUCCESS) {
     DERROR("Unable to create vulkan buffer beacause the required memory allocation failed: '%s'", result);
     return false;
   }
   
   if (bind_on_create) {
     vulkan_buffer_bind(context, out_buffer, 0);
   }
   
   return true;
}

void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer) {
  if (buffer->memory) {
    vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);
    buffer->memory = 0;
  }
  if (buffer->handle) {
    vkDestroyBuffer(context->device.logical_device, buffer->handle, context->allocator);
    buffer->handle = 0;
  }
  buffer->total_size = 0;
  buffer->usage = 0;
  buffer->is_locked = false;
}

bool vulkan_buffer_resize(vulkan_context* context, uint64_t new_size, vulkan_buffer* buffer, VkQueue queue, VkCommandPool pool) {
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = new_size;
  buffer_info.usage = buffer->usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 

  VkBuffer new_buffer;
  VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &new_buffer));
  
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(context->device.logical_device, new_buffer, &requirements);
  
  VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = (uint32_t)buffer->memory_index;

  VkDeviceMemory new_memory;
  VkResult result = vkAllocateMemory(context->device.logical_device, &allocate_info, context->allocator, &new_memory);
  if (result != VK_SUCCESS) {
    DERROR("Unable to resize vulkan buffer because the required memory allocation failed. Error: %i", result);
    return false;
  }

  VK_CHECK(vkBindBufferMemory(context->device.logical_device, new_buffer, new_memory, 0));
  
  vulkan_buffer_copy_to(context, pool, 0, queue, buffer->handle, 0, new_buffer, 0, buffer->total_size);
  
  vkDeviceWaitIdle(context->device.logical_device);
  
  if (buffer->memory) {
    vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);
    buffer->memory = 0;
  }
  if (buffer->handle) {
    vkDestroyBuffer(context->device.logical_device, buffer->handle, context->allocator);
    buffer->handle = 0;
  }
  
  buffer->total_size = new_size;
  buffer->memory = new_memory;
  buffer->handle = new_buffer;
  
  return true;
}

void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset) {
  VK_CHECK(vkBindBufferMemory(context->device.logical_device, buffer->handle, buffer->memory, offset));
}

void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset, uint64_t size, uint32_t flags) {
  void* data;
  VK_CHECK(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &data));
  return data;
}

void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer) {
  vkUnmapMemory(context->device.logical_device, buffer->memory);
}

void vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, uint64_t offset, uint64_t size, uint32_t flags, const void* data) {
  void* data_ptr;
  VK_CHECK(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &data_ptr));
  memcpy(data_ptr, data, size);
  vkUnmapMemory(context->device.logical_device, buffer->memory);
}

void vulkan_buffer_copy_to(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, uint64_t source_offset, VkBuffer dest, uint64_t dest_offset, uint64_t size) {
  vkQueueWaitIdle(queue);
  vulkan_command_buffer temp_command_buffer;
  vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &temp_command_buffer);
  
  VkBufferCopy copy_region;
  copy_region.srcOffset = source_offset;
  copy_region.dstOffset = dest_offset;
  copy_region.size = size;
  
  vkCmdCopyBuffer(temp_command_buffer.handle, source, dest, 1, &copy_region);
  
  vulkan_command_buffer_end_single_use(context, pool, &temp_command_buffer, queue);
}
