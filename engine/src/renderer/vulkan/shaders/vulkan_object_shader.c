#include "vulkan_object_shader.h"
#include "core/logger.h"
#include "core/dmemory.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include <string.h>

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"
#define OBJECT_SHADER_STAGE_COUNT 2
#define OBJECT_SHADER_DESCRIPTOR_COUNT 3

bool vulkan_object_shader_create(vulkan_context* context, vulkan_object_shader* out_shader) {
  char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
  VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT};

  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i],
                              stage_types[i], i, out_shader->stages)) {
      DERROR("Unable to create %s shader module for '%s'", stage_type_strs[i],
             BUILTIN_SHADER_NAME_OBJECT);
      return false;
    }
  }

  VkDescriptorSetLayoutBinding global_ubo_layout_binding;
  global_ubo_layout_binding.binding = 0;
  global_ubo_layout_binding.descriptorCount = 1;
  global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  global_ubo_layout_binding.pImmutableSamplers = 0;
  global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo global_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  global_layout_info.bindingCount = 1;
  global_layout_info.pBindings = &global_ubo_layout_binding;
  VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device, &global_layout_info,
                                       context->allocator, &out_shader->global_descriptor_set_layout));

  VkDescriptorPoolSize pool_size;
  pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size.descriptorCount = OBJECT_SHADER_DESCRIPTOR_COUNT;

  VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = OBJECT_SHADER_DESCRIPTOR_COUNT;
  VK_CHECK(vkCreateDescriptorPool(context->device.logical_device, &pool_info, context->allocator,
                                  &out_shader->global_descriptor_pool));

  VkDescriptorSetLayout layouts[OBJECT_SHADER_DESCRIPTOR_COUNT] = {
      out_shader->global_descriptor_set_layout,
      out_shader->global_descriptor_set_layout,
      out_shader->global_descriptor_set_layout};

  VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  alloc_info.descriptorPool = out_shader->global_descriptor_pool;
  alloc_info.descriptorSetCount = OBJECT_SHADER_DESCRIPTOR_COUNT;
  alloc_info.pSetLayouts = layouts;
  VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device, &alloc_info,
                                    out_shader->global_descriptor_sets));

  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = (float)context->framebuffer_height;
  viewport.width = (float)context->framebuffer_width;
  viewport.height = -(float)context->framebuffer_height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = context->framebuffer_width;
  scissor.extent.height = context->framebuffer_height;

  VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];
  memset(stage_create_infos, 0, sizeof(stage_create_infos));
  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
  }

  VkVertexInputAttributeDescription attribute_descriptions[1];
  memset(attribute_descriptions, 0, sizeof(attribute_descriptions));
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(vertex_3d, position);

  if (!vulkan_graphics_pipeline_create(
          context,
          &context->main_renderpass,
          1,
          attribute_descriptions,
          1,
          &out_shader->global_descriptor_set_layout,
          OBJECT_SHADER_STAGE_COUNT,
          stage_create_infos,
          viewport,
          scissor,
          false,
          &out_shader->pipeline)) {
    DERROR("Failed to create graphics pipeline for object shader.");
    return false;
  }

  if (!vulkan_buffer_create(
          context,
          sizeof(global_uniform_object),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          true,
          &out_shader->global_uniform_buffer)) {
    DERROR("Failed to create object shader global UBO buffer.");
    return false;
  }

  return true;
}

void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader) {
  vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
  vulkan_pipeline_destroy(context, &shader->pipeline);
  vkDestroyDescriptorPool(context->device.logical_device, shader->global_descriptor_pool,
                          context->allocator);
  vkDestroyDescriptorSetLayout(context->device.logical_device, shader->global_descriptor_set_layout,
                               context->allocator);

  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    vkDestroyShaderModule(context->device.logical_device, shader->stages[i].handle,
                          context->allocator);
    shader->stages[i].handle = 0;
  }
}

void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader) {
  uint32_t image_index = context->image_index;
  vulkan_pipeline_bind(&context->graphics_command_buffers[image_index],
                       VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vulkan_object_shader_update_global_state(vulkan_context* context,
                                              struct vulkan_object_shader* shader) {
  uint32_t frame_index = context->current_frame;

  if (frame_index >= OBJECT_SHADER_DESCRIPTOR_COUNT) {
    DERROR("current_frame out of range for global descriptor sets (%u >= %u)",
           frame_index, OBJECT_SHADER_DESCRIPTOR_COUNT);
    return;
  }

  VkCommandBuffer command_buffer =
      context->graphics_command_buffers[context->image_index].handle;

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shader->pipeline.pipeline_layout, 0, 1,
                          &shader->global_descriptor_sets[frame_index], 0, 0);

  vulkan_buffer_load_data(context, &shader->global_uniform_buffer, 0,
                          sizeof(global_uniform_object), 0, &shader->global_ubo);

  VkDescriptorBufferInfo buffer_info;
  buffer_info.buffer = shader->global_uniform_buffer.handle;
  buffer_info.offset = 0;
  buffer_info.range = sizeof(global_uniform_object);

  VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptor_write.dstSet = shader->global_descriptor_sets[frame_index];
  descriptor_write.dstBinding = 0;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pBufferInfo = &buffer_info;

  vkUpdateDescriptorSets(context->device.logical_device, 1, &descriptor_write, 0, 0);
}

void vulkan_object_shader_update_object(vulkan_context* context, struct vulkan_object_shader* shader, mat4 model) {
  uint32_t image_index = context->image_index;
  VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
  vkCmdPushConstants(command_buffer, shader->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
}
