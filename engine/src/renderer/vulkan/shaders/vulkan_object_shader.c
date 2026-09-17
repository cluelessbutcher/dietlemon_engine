#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "math/math_types.h"

#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"

#include <string.h>

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

bool vulkan_object_shader_create(vulkan_context* context, vulkan_object_shader* out_shader) {
  char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
  VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
  
  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {
      DERROR("Unable to create %s shader module for '%s'", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);
      return false;
    }
  }
  
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

  uint32_t offset = 0;
  const int32_t attribute_count = 1;
  VkVertexInputAttributeDescription attribute_descriptions[attribute_count];
  
  VkFormat formats[attribute_count] = {
    VK_FORMAT_R32G32B32_SFLOAT
  };
  uint64_t sizes[attribute_count] = {
    sizeof(vec3)
  };
  
  for (uint32_t i = 0; i < attribute_count; ++i) {
    attribute_descriptions[i].binding = 0;
    attribute_descriptions[i].location = i;
    attribute_descriptions[i].format = formats[i];
    attribute_descriptions[i].offset = offset;
    offset += sizes[i];
  }
  
  VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];
  memset(stage_create_infos, 0, sizeof(stage_create_infos));
  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    stage_create_infos[i].sType = out_shader->stages[i].shader_stage_create_info.sType;
    stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
  }

  if (!vulkan_graphics_pipeline_create(context,
                                       &context->main_renderpass,
                                       attribute_count,
                                       attribute_descriptions,
                                       0,
                                       0,
                                       OBJECT_SHADER_STAGE_COUNT,
                                       stage_create_infos,
                                       viewport,
                                       scissor,
                                       false,
                                       &out_shader->pipeline)) {
    DERROR("Failed to load graphics pipeline for object shader.");
    return false;
  }
  return true;
}

void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader) {
  vulkan_pipeline_destroy(context, &shader->pipeline);
  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    vkDestroyShaderModule(context->device.logical_device, shader->stages[i].handle, context->allocator);
    shader->stages[i].handle = 0;
  }
}

void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader) {

}
