#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name);
void renderer_system_shutdown(void* state);
void renderer_on_resize(uint16_t width, uint16_t height);
bool renderer_draw_frame(render_packet* packet);
