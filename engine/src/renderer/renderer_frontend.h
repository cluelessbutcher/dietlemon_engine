#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name);
void renderer_system_shutdown(void* state);
void renderer_on_resized(uint16_t width, uint16_t height);
bool renderer_draw_frame(render_packet* packet);

DAPI void renderer_set_view(mat4 view);

void renderer_create_texture(const char* name, bool auto_release, int32_t width, int32_t height, int32_t channel_count, const uint8_t* pixels, bool has_transparency, struct texture* out_texture);
void renderer_destroy_texture(struct texture* texture);
