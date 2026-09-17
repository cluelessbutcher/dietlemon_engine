#pragma once

#include "vulkan_types.inl"

bool vulkan_graphics_pipeline_create(vulkan_context* context,
                                     vulkan_renderpass* renderpass,
                                     uint32_t attribute_count,
                                     VkVertexInputAttributeDescription* attributes,
                                     uint32_t descriptor_set_layout_count,
                                     VkDescriptorSetLayout* descriptor_set_layouts,
                                     uint32_t stage_count,
                                     VkPipelineShaderStageCreateInfo* stages,
                                     VkViewport viewport,
                                     VkRect2D scissor,
                                     bool is_wireframe,
                                     vulkan_pipeline* out_pipeline);

void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline);
void vulkan_pipeline_bind(vulkan_command_buffer* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline);
