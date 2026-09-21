#include "vulkan_shader_utils.h"

#include "core/dstring.h"
#include "core/logger.h"
#include "core/dmemory.h"

#include "platform/filesystem.h"

#include <string.h>

bool create_shader_module(vulkan_context* context,
                          const char* name,
                          const char* type_str,
                          VkShaderStageFlagBits shader_stage_flag,
                          uint32_t stage_index,
                          vulkan_shader_stage* shader_stages) {
  char file_name[512];
  string_format(file_name, "assets/shaders/%s.%s.spv", name, type_str);
  memset(&shader_stages[stage_index].create_info, 0, sizeof(VkShaderModuleCreateInfo));
  shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 
  file_handle handle;
  if (!filesystem_open(file_name, FILE_MODE_READ, true, &handle)) {
    DERROR("Unable to read shader module: %s", file_name);
    return false;
  }
  
  uint64_t size = 0;
  uint8_t* file_buffer = 0;
  if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {
    DERROR("Unable to binary read shader module: %s", file_name);
    return false;
  }
  shader_stages[stage_index].create_info.codeSize = size;
  shader_stages[stage_index].create_info.pCode = (uint32_t*)file_buffer;
  
  filesystem_close(&handle);
  
  VK_CHECK(vkCreateShaderModule(context->device.logical_device, &shader_stages[stage_index].create_info, context->allocator, &shader_stages[stage_index].handle));
  memset(&shader_stages[stage_index].shader_stage_create_info, 0, sizeof(VkPipelineShaderStageCreateInfo));
  shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;
  shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;
  shader_stages[stage_index].shader_stage_create_info.pName = "main";
  
  return true;
}
