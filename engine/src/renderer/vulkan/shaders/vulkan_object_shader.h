#pragma once 

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

bool vulkan_object_shader_create(vulkan_context* context, texture* default_diffuse, vulkan_object_shader* out_shader);
void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader);
void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader);
void vulkan_object_shader_update_global_state(vulkan_context* context, struct vulkan_object_shader* shader, float delta_time);
void vulkan_object_shader_update_object(vulkan_context* context, struct vulkan_object_shader* shader, geometry_render_data data);
bool vulkan_object_shader_acquire_resources(vulkan_context* context, struct vulkan_object_shader* shader, uint32_t* out_object_id);
void vulkan_object_shader_release_resources(vulkan_context* context, struct vulkan_object_shader* shader, uint32_t object_id);
