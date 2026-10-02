#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name);
void renderer_system_shutdown(void* state);
void renderer_on_resized(uint16_t width, uint16_t height);
bool renderer_draw_frame(render_packet* packet);

DAPI void renderer_set_view(mat4 view);

void renderer_create_texture(const uint8_t* pixels, struct texture* out_texture);
void renderer_destroy_texture(struct texture* texture);
bool renderer_create_material(struct material* material);
void renderer_destroy_material(struct material* material);

bool renderer_create_geometry(geometry* geometry, uint32_t vertex_count, const vertex_3d* vertices, uint32_t index_count, const uint32_t* indices);
void renderer_destroy_geometry(geometry* geometry);
