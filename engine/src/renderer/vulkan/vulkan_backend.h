#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name);
void vulkan_renderer_backend_shutdown(renderer_backend* backend);
void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height);
bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, float delta_time);
void vulkan_renderer_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, int32_t mode);
bool vulkan_renderer_backend_end_frame(renderer_backend* backend, float delta_time);
void vulkan_backend_update_object(geometry_render_data data);
void vulkan_renderer_create_texture(const char* name, bool auto_release, int32_t width, int32_t height, int32_t channel_count, const uint8_t* pixels, bool has_transparency, texture* out_texture);
void vulkan_renderer_destroy_texture(texture* texture);
