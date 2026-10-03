#include "vulkan_shader_utils.h"

#include "core/dstring.h"
#include "core/logger.h"
#include "core/dmemory.h"

#include "systems/resource_system.h"

#include <string.h>

bool create_shader_module(vulkan_context* context,
                          const char* name,
                          const char* type_str,
                          VkShaderStageFlagBits shader_stage_flag,
                          uint32_t stage_index,
                          vulkan_shader_stage* shader_stages) {
  char file_name[512];
  string_format(file_name, "shaders/%s.%s.spv", name, type_str);
  
  resource binary_resource;
  if (!resource_system_load(file_name, RESOURCE_TYPE_BINARY, &binary_resource)) {
    DERROR("Unable to read shader module: %s", file_name);
    return false;
  }
  
  memset(&shader_stages[stage_index].create_info, 0, sizeof(VkShaderModuleCreateInfo));
  shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_stages[stage_index].create_info.codeSize = binary_resource.data_size;
  shader_stages[stage_index].create_info.pCode = (uint32_t*)binary_resource.data;
  
  VK_CHECK(vkCreateShaderModule(context->device.logical_device, &shader_stages[stage_index].create_info, context->allocator, &shader_stages[stage_index].handle));

  resource_system_unload(&binary_resource);
  
  memset(&shader_stages[stage_index].shader_stage_create_info, 0, sizeof(VkPipelineShaderStageCreateInfo));
  shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;
  shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;
  shader_stages[stage_index].shader_stage_create_info.pName = "main";
  
  return true;
}
