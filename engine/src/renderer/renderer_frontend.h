#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

bool renderer_initialize(const char* application_name, struct platform_state* plat_state);
void renderer_shutdown();
void renderer_on_resize(uint16_t width, uint16_t height);
bool renderer_draw_frame(render_packet* packet);
