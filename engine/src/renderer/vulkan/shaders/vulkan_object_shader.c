#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "math/math_types.h"
#include "math/dmath.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_buffer.h"

#include <string.h>

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"
#define OBJECT_SHADER_DESCRIPTOR_SET_COUNT 3

bool vulkan_object_shader_create(vulkan_context* context, texture* default_diffuse, vulkan_object_shader* out_shader) {
  out_shader->default_diffuse = default_diffuse;
  
  char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
  VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {
    VK_SHADER_STAGE_VERTEX_BIT,
    VK_SHADER_STAGE_FRAGMENT_BIT
  };

  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {
      DERROR("Unable to create %s shader module for '%s'.", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);
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
  VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device, &global_layout_info, context->allocator, &out_shader->global_descriptor_set_layout));

  VkDescriptorPoolSize global_pool_size;
  global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  global_pool_size.descriptorCount = OBJECT_SHADER_DESCRIPTOR_SET_COUNT;

  VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  global_pool_info.poolSizeCount = 1;
  global_pool_info.pPoolSizes = &global_pool_size;
  global_pool_info.maxSets = OBJECT_SHADER_DESCRIPTOR_SET_COUNT;
  VK_CHECK(vkCreateDescriptorPool(context->device.logical_device, &global_pool_info, context->allocator, &out_shader->global_descriptor_pool));

  const uint32_t local_sampler_count = 1;
  VkDescriptorType descriptor_types[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] = {
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  };

  VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
  memset(bindings, 0, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);
  for (uint32_t i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
    bindings[i].binding = i;
    bindings[i].descriptorCount = 1;
    bindings[i].descriptorType = descriptor_types[i];
    bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;
  layout_info.pBindings = bindings;
  VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device, &layout_info, context->allocator, &out_shader->object_descriptor_set_layout));

  VkDescriptorPoolSize object_pool_sizes[2];
  object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  object_pool_sizes[0].descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT;
  object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  object_pool_sizes[1].descriptorCount = local_sampler_count * VULKAN_OBJECT_MAX_OBJECT_COUNT;

  VkDescriptorPoolCreateInfo object_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  object_pool_info.poolSizeCount = 2;
  object_pool_info.pPoolSizes = object_pool_sizes;
  object_pool_info.maxSets = VULKAN_OBJECT_MAX_OBJECT_COUNT;
  VK_CHECK(vkCreateDescriptorPool(context->device.logical_device, &object_pool_info, context->allocator, &out_shader->object_descriptor_pool));

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
#define ATTRIBUTE_COUNT 2
  VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT];
  VkFormat formats[ATTRIBUTE_COUNT] = {
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT
  };
  uint64_t sizes[ATTRIBUTE_COUNT] = {
    sizeof(vec3),
    sizeof(vec2)
  };
  for (uint32_t i = 0; i < ATTRIBUTE_COUNT; ++i) {
    attribute_descriptions[i].binding = 0;
    attribute_descriptions[i].location = i;
    attribute_descriptions[i].format = formats[i];
    attribute_descriptions[i].offset = offset;
    offset += sizes[i];
  }

  const int32_t descriptor_set_layout_count = 2;
  VkDescriptorSetLayout layouts[2] = {
    out_shader->global_descriptor_set_layout,
    out_shader->object_descriptor_set_layout
  };

  VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];
  memset(stage_create_infos, 0, sizeof(stage_create_infos));
  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
  }

  if (!vulkan_graphics_pipeline_create(
                                       context,
                                       &context->main_renderpass,
                                       ATTRIBUTE_COUNT,
                                       attribute_descriptions,
                                       descriptor_set_layout_count,
                                       layouts,
                                       OBJECT_SHADER_STAGE_COUNT,
                                       stage_create_infos,
                                       viewport,
                                       scissor,
                                       false,
                                       &out_shader->pipeline)) {
    DERROR("Failed to load graphics pipeline for object shader.");
    return false;
  }

  if (!vulkan_buffer_create(
                            context,
                            sizeof(global_uniform_object),
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            true,
                            &out_shader->global_uniform_buffer)) {
    DERROR("Vulkan buffer creation failed for object shader.");
    return false;
  }

  VkDescriptorSetLayout global_layouts[OBJECT_SHADER_DESCRIPTOR_SET_COUNT] = {
    out_shader->global_descriptor_set_layout,
    out_shader->global_descriptor_set_layout,
    out_shader->global_descriptor_set_layout
  };

  VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  alloc_info.descriptorPool = out_shader->global_descriptor_pool;
  alloc_info.descriptorSetCount = OBJECT_SHADER_DESCRIPTOR_SET_COUNT;
  alloc_info.pSetLayouts = global_layouts;
  VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, out_shader->global_descriptor_sets));

  if (!vulkan_buffer_create(
                            context,
                            sizeof(object_uniform_object) * VULKAN_OBJECT_MAX_OBJECT_COUNT,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            true,
                            &out_shader->object_uniform_buffer)) {
    DERROR("Object uniform buffer creation failed for shader.");
    return false;
  }

  out_shader->object_uniform_buffer_index = 0;
  memset(out_shader->object_states, 0, sizeof(out_shader->object_states));

  return true;
}

void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader) {
  VkDevice logical_device = context->device.logical_device;

  vkDestroyDescriptorPool(logical_device, shader->object_descriptor_pool, context->allocator);
  vkDestroyDescriptorSetLayout(logical_device, shader->object_descriptor_set_layout, context->allocator);

  vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
  vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

  vulkan_pipeline_destroy(context, &shader->pipeline);

  vkDestroyDescriptorPool(logical_device, shader->global_descriptor_pool, context->allocator);
  vkDestroyDescriptorSetLayout(logical_device, shader->global_descriptor_set_layout, context->allocator);

  for (uint32_t i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    vkDestroyShaderModule(logical_device, shader->stages[i].handle, context->allocator);
    shader->stages[i].handle = 0;
  }
}

void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader) {
  uint32_t image_index = context->image_index;
  vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vulkan_object_shader_update_global_state(vulkan_context* context, struct vulkan_object_shader* shader, float delta_time) {
  uint32_t frame_index = context->current_frame;
  if (frame_index >= OBJECT_SHADER_DESCRIPTOR_SET_COUNT) {
    DERROR("current_frame out of range for global descriptor sets (%u >= %u)", frame_index, OBJECT_SHADER_DESCRIPTOR_SET_COUNT);
    return;
  }

  VkCommandBuffer command_buffer = context->graphics_command_buffers[context->image_index].handle;
  VkDescriptorSet global_descriptor = shader->global_descriptor_sets[frame_index];

  vkCmdBindDescriptorSets(
                          command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shader->pipeline.pipeline_layout,
                          0, 1,
                          &global_descriptor,
                          0, 0);

  uint32_t range = sizeof(global_uniform_object);
  uint64_t offset = 0;

  vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

  VkDescriptorBufferInfo buffer_info;
  buffer_info.buffer = shader->global_uniform_buffer.handle;
  buffer_info.offset = offset;
  buffer_info.range = range;

  VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptor_write.dstSet = global_descriptor;
  descriptor_write.dstBinding = 0;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pBufferInfo = &buffer_info;

  vkUpdateDescriptorSets(context->device.logical_device, 1, &descriptor_write, 0, 0);
}

void vulkan_object_shader_update_object(vulkan_context* context, struct vulkan_object_shader* shader, geometry_render_data data) {
  uint32_t image_index = context->image_index;
  uint32_t frame_index = context->current_frame;
  VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

  vkCmdPushConstants(
                     command_buffer,
                     shader->pipeline.pipeline_layout,
                     VK_SHADER_STAGE_VERTEX_BIT,
                     0,
                     sizeof(mat4),
                     &data.model);

  vulkan_object_shader_object_state* object_state = &shader->object_states[data.object_id];
  VkDescriptorSet object_descriptor_set = object_state->descriptor_sets[frame_index];

  VkWriteDescriptorSet descriptor_writes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
  memset(descriptor_writes, 0, sizeof(VkWriteDescriptorSet) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);
  uint32_t descriptor_count = 0;
  uint32_t descriptor_index = 0;

  uint32_t range = sizeof(object_uniform_object);
  uint64_t offset = sizeof(object_uniform_object) * data.object_id;

  object_uniform_object obo;
  static float accumulator = 0.0f;
  accumulator += context->frame_delta_time;
  float s = (dsin(accumulator) + 1.0f) / 2.0f;
  obo.diffuse_color = vec4_create(s, s, s, 1.0f);

  vulkan_buffer_load_data(context, &shader->object_uniform_buffer, offset, range, 0, &obo);

  if (object_state->descriptor_states[descriptor_index].generations[frame_index] == INVALID_ID) {
    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = shader->object_uniform_buffer.handle;
    buffer_info.offset = offset;
    buffer_info.range = range;

    VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor.dstSet = object_descriptor_set;
    descriptor.dstBinding = descriptor_index;
    descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor.descriptorCount = 1;
    descriptor.pBufferInfo = &buffer_info;

    descriptor_writes[descriptor_count] = descriptor;
    descriptor_count++;

    object_state->descriptor_states[descriptor_index].generations[frame_index] = 1;
  }
  descriptor_index++;

  const uint32_t sampler_count = 1;
  VkDescriptorImageInfo image_infos[1];
  for (uint32_t sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {
    texture* t = data.textures[sampler_index];
    uint32_t* descriptor_generation = &object_state->descriptor_states[descriptor_index].generations[frame_index];

    if (t->generation == INVALID_ID) {
      t = shader->default_diffuse;
      *descriptor_generation = INVALID_ID;
    }
    
    if (t && (*descriptor_generation != t->generation || *descriptor_generation == INVALID_ID)) {
      vulkan_texture_data* internal_data = (vulkan_texture_data*)t->internal_data;

      image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image_infos[sampler_index].imageView = internal_data->image.view;
      image_infos[sampler_index].sampler = internal_data->sampler;

      VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      descriptor.dstSet = object_descriptor_set;
      descriptor.dstBinding = descriptor_index;
      descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor.descriptorCount = 1;
      descriptor.pImageInfo = &image_infos[sampler_index];

      descriptor_writes[descriptor_count] = descriptor;
      descriptor_count++;

      if (t->generation != INVALID_ID) {
        *descriptor_generation = t->generation;
      }
      descriptor_index++;
    }
  }

  if (descriptor_count > 0) {
    vkUpdateDescriptorSets(context->device.logical_device, descriptor_count, descriptor_writes, 0, 0);
  }

  vkCmdBindDescriptorSets(
                          command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shader->pipeline.pipeline_layout,
                          1, 1,
                          &object_descriptor_set,
                          0, 0);
}

bool vulkan_object_shader_acquire_resources(vulkan_context* context, struct vulkan_object_shader* shader, uint32_t* out_object_id) {
  *out_object_id = shader->object_uniform_buffer_index;
  shader->object_uniform_buffer_index++;

  uint32_t object_id = *out_object_id;
  vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];

  for (uint32_t i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
    for (uint32_t j = 0; j < OBJECT_SHADER_DESCRIPTOR_SET_COUNT; ++j) {
      object_state->descriptor_states[i].generations[j] = INVALID_ID;
    }
  }

  VkDescriptorSetLayout layouts[OBJECT_SHADER_DESCRIPTOR_SET_COUNT] = {
    shader->object_descriptor_set_layout,
    shader->object_descriptor_set_layout,
    shader->object_descriptor_set_layout
  };

  VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  alloc_info.descriptorPool = shader->object_descriptor_pool;
  alloc_info.descriptorSetCount = OBJECT_SHADER_DESCRIPTOR_SET_COUNT;
  alloc_info.pSetLayouts = layouts;

  VkResult result = vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, object_state->descriptor_sets);
  if (result != VK_SUCCESS) {
    DERROR("Error allocating descriptor sets in shader!");
    return false;
  }

  return true;
}

void vulkan_object_shader_release_resources(vulkan_context* context, struct vulkan_object_shader* shader, uint32_t object_id) {
  vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];

  const uint32_t descriptor_set_count = OBJECT_SHADER_DESCRIPTOR_SET_COUNT;

  VkResult result = vkFreeDescriptorSets(
                                         context->device.logical_device,
                                         shader->object_descriptor_pool,
                                         descriptor_set_count,
                                         object_state->descriptor_sets);

  if (result != VK_SUCCESS) {
    DERROR("Error freeing object shader descriptor sets!");
  }

  for (uint32_t i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
    for (uint32_t j = 0; j < OBJECT_SHADER_DESCRIPTOR_SET_COUNT; ++j) {
      object_state->descriptor_states[i].generations[j] = INVALID_ID;
    }
  }
}
