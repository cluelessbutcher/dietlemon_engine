#pragma once

#include "defines.h"

#include "core/asserts.h"

#include "renderer/renderer_types.inl"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                          \
  {                                             \
    DASSERT(expr == VK_SUCCESS);                \
  }

typedef struct vulkan_buffer {
  uint64_t total_size;
  VkBuffer handle;
  VkBufferUsageFlagBits usage;
  bool is_locked;
  VkDeviceMemory memory;
  int32_t memory_index;
  uint32_t memory_property_flags;
} vulkan_buffer;

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

typedef struct vulkan_shader_stage {
  VkShaderModuleCreateInfo create_info;
  VkShaderModule handle;
  VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

typedef struct vulkan_pipeline {
  VkPipeline handle;
  VkPipelineLayout pipeline_layout;
} vulkan_pipeline;

#define MATERIAL_SHADER_STAGE_COUNT 2

typedef struct vulkan_descriptor_state {
  uint32_t generations[3];
  uint32_t ids[3];
} vulkan_descriptor_state;

#define VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_MATERIAL_SHADER_SAMPLER_COUNT 1

typedef struct vulkan_material_shader_instance_state {
  VkDescriptorSet descriptor_sets[3];
  vulkan_descriptor_state descriptor_states[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
} vulkan_material_shader_instance_state;

#define VULKAN_MAX_GEOMETRY_COUNT 1024
#define VULKAN_MAX_MATERIAL_COUNT 4096

typedef struct vulkan_geometry_data {
  uint32_t id;
  uint32_t generation;
  uint32_t vertex_count;
  uint32_t vertex_size;
  uint32_t vertex_buffer_offset;
  uint32_t index_count;
  uint32_t index_size;
  uint32_t index_buffer_offset;
} vulkan_geometry_data;

typedef struct vulkan_material_shader {
  vulkan_shader_stage stages[MATERIAL_SHADER_STAGE_COUNT];
  VkDescriptorPool global_descriptor_pool;
  VkDescriptorSetLayout global_descriptor_set_layout;
  VkDescriptorSet global_descriptor_sets[3];
  global_uniform_object global_ubo;
  vulkan_buffer global_uniform_buffer;
  VkDescriptorPool object_descriptor_pool;
  VkDescriptorSetLayout object_descriptor_set_layout;
  vulkan_buffer material_uniform_buffer;
  uint32_t material_uniform_buffer_index;
  texture_use sampler_uses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];
  vulkan_material_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];
  vulkan_pipeline pipeline;
} vulkan_material_shader;

typedef struct vulkan_context {
  float frame_delta_time;
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

  vulkan_buffer object_vertex_buffer;
  vulkan_buffer object_index_buffer;

  vulkan_command_buffer* graphics_command_buffers;

  VkSemaphore* image_available_semaphores;
  VkSemaphore* queue_complete_semaphores;
  uint32_t in_flight_fence_count;
  vulkan_fence* in_flight_fences;
  vulkan_fence** images_in_flight;

  uint32_t image_index;
  uint32_t current_frame;
  bool recreating_swapchain;
  vulkan_material_shader material_shader;

  uint64_t geometry_vertex_offset;
  uint64_t geometry_index_offset;

  vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT];
  
  int32_t (*find_memory_index)(uint32_t type_filter, uint32_t property_flags);
} vulkan_context;

typedef struct vulkan_texture_data {
  vulkan_image image;
  VkSampler sampler;
} vulkan_texture_data;
