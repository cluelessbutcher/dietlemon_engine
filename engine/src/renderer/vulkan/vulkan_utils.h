#pragma once

#include "vulkan_types.inl"

const char* vulkan_result_string(VkResult reslut, bool get_extended);
bool vulkan_result_is_success(VkResult result);