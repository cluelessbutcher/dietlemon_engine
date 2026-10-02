#pragma once

#include "renderer/renderer_types.inl"

typedef struct geometry_system_config {
  uint32_t max_geometry_count;
} geometry_system_config;

typedef struct geometry_config {
  uint32_t vertex_count;
  vertex_3d* vertices;
  uint32_t index_count;
  uint32_t* indices;
  char name[GEOMETRY_NAME_MAX_LENGTH];
  char material_name[MATERIAL_NAME_MAX_LENGTH];
} geometry_config;

#define DEFAULT_GEOMETRY_NAME "default"

bool geometry_system_initialize(uint64_t* memory_requirement, void* state, geometry_system_config config);
void geometry_system_shutdown(void* state);

geometry* geometry_system_acquire_by_id(uint32_t id);
geometry* geometry_system_acquire_from_config(geometry_config config, bool auto_release);
void geometry_system_release(geometry* geometry);
geometry* geometry_system_get_default();
geometry_config geometry_system_generate_plane_config(float width, float height, uint32_t x_segment_count, uint32_t y_segment_count, float tile_x, float tile_y, const char* name, const char* material_name);
