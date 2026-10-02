#include "geometry_system.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/dstring.h"
#include "systems/material_system.h"
#include "renderer/renderer_frontend.h"

#include <string.h>

typedef struct geometry_reference {
  uint64_t reference_count;
  geometry geometry;
  bool auto_release;
} geometry_reference;

typedef struct geometry_system_state {
  geometry_system_config config;
  geometry default_geometry;
  geometry_reference* registered_geometries;
} geometry_system_state;

static geometry_system_state* state_ptr = 0;

bool create_default_geometry(geometry_system_state* state);
bool create_geometry(geometry_system_state* state, geometry_config config, geometry* g);
void destroy_geometry(geometry_system_state* state, geometry* g);

bool geometry_system_initialize(uint64_t* memory_requirement, void* state, geometry_system_config config) {
  if (config.max_geometry_count == 0) {
    DFATAL("geometry_system_initialize() - config.max_geometry_count must be > 0");
    return false;
  }

  uint64_t struct_requirement = sizeof(geometry_system_state);
  uint64_t array_requirement = sizeof(geometry) * config.max_geometry_count;
  *memory_requirement = struct_requirement * array_requirement;

  if (!state) {
    return true;
  }

  state_ptr = state;
  state_ptr->config = config;

  void* array_block = state + struct_requirement;
  state_ptr->registered_geometries = array_block;

  uint32_t count = state_ptr->config.max_geometry_count;
  for (uint32_t i =0; i < count; ++i) {
    state_ptr->registered_geometries[i].geometry.id = INVALID_ID;
    state_ptr->registered_geometries[i].geometry.internal_id = INVALID_ID;
    state_ptr->registered_geometries[i].geometry.generation = INVALID_ID;
  }

  if (!create_default_geometry(state_ptr)) {
    DFATAL("Failed to create default geometry; applicatoin cannot continue");
    return false;
  }

  return true;
}

void geometry_system_shutdown(void* state) {}

geometry* geometry_system_acquire_by_id(uint32_t id) {
  if (id != INVALID_ID && state_ptr->registered_geometries[id].geometry.id != INVALID_ID) {
    state_ptr->registered_geometries[id].reference_count++;
    return &state_ptr->registered_geometries[id].geometry;
  }

  DERROR("geometry_system_acquire_by_id() canoot load invalid geometry id; returning nullptr");
  return 0;
}

geometry* geometry_system_acquire_from_config(geometry_config config, bool auto_release) {
  geometry* g = 0;
  for (uint32_t i = 0; i < state_ptr->config.max_geometry_count; ++i) {
    if (state_ptr->registered_geometries[i].geometry.id == INVALID_ID) {
      state_ptr->registered_geometries[i].auto_release = auto_release;
      state_ptr->registered_geometries[i].reference_count = 1;
      g = &state_ptr->registered_geometries[i].geometry;
      g->id = i;
      break;
    }
  }

  if (!g) {
    DERROR("Unable to obtain free slot for geometry; adjust configuration to allow more space; returning nullptr");
    return 0;
  }

  if (!create_geometry(state_ptr, config, g)) {
    DERROR("Failed to create geometry; returning nullptr");
    return 0;
  }

  return g;
}

void geometry_system_release(geometry* geometry) {
  if (geometry && geometry->id != INVALID_ID) {
    geometry_reference* ref = &state_ptr->registered_geometries[geometry->id];

    uint32_t id = geometry->id;
    if (ref->geometry.id == geometry->id) {
      if (ref->reference_count > 0) {
	ref->reference_count--;
      }

      if (ref->reference_count < 1 && ref->auto_release) {
	destroy_geometry(state_ptr, &ref->geometry);
	ref->reference_count = 0;
	ref->auto_release = false;
      }
    } else {
      DFATAL("Geometry id mismatch; check registeration logic, as this should never occur");
    }
    return;
  }

  DWARN("geometry_system_acquire_by_id cannot release invalid geometry id; nothing was done");
}

geometry* geometry_system_get_default() {
  if (state_ptr) {
    return &state_ptr->default_geometry;
  }

  DFATAL("geometry_system_get_default called before system was initialized; retuning nullptr");
  return 0;
}

bool create_geometry(geometry_system_state* state, geometry_config config, geometry* g) {
  if (!renderer_create_geometry(g, config.vertex_count, config.vertices, config.index_count, config.indices)) {
    state->registered_geometries[g->id].reference_count = 0;
    state->registered_geometries[g->id].auto_release = false;
    g->id = INVALID_ID;
    g->generation = INVALID_ID;
    g->internal_id = INVALID_ID;

    return false;
  }

  if (strlen(config.material_name) > 0) {
    g->material = material_system_acquire(config.material_name);
    if (!g->material) {
      g->material = material_system_get_default();
    }
  }

  return true;
}

void destroy_geometry(geometry_system_state* state, geometry* g) {
  renderer_destroy_geometry(g);
  g->internal_id = INVALID_ID;
  g->generation = INVALID_ID;
  g->id = INVALID_ID;

  string_empty(g->name);

  if (g->material && strlen(g->material->name) > 0) {
    material_system_release(g->material->name);
    g->material = 0;
  }
}

bool create_default_geometry(geometry_system_state* state) {
  vertex_3d verts[4];
  memset(verts, 0, sizeof(vertex_3d) * 4);

  const float f = 10.0f;

  verts[0].position.x = -0.5 * f;
  verts[0].position.y = -0.5 * f;
  verts[0].texcoord.x = 0.0f;
  verts[0].texcoord.y = 0.0f;

  verts[1].position.x = 0.5 * f;
  verts[1].position.y = 0.5 * f;
  verts[1].texcoord.x = 1.0f;
  verts[1].texcoord.y = 1.0f;

  verts[2].position.x = -0.5 * f;
  verts[2].position.y = 0.5 * f;
  verts[2].texcoord.x = 0.0f;
  verts[2].texcoord.y = 1.0f;

  verts[3].position.x = 0.5 * f;
  verts[3].position.y = -0.5 * f;
  verts[3].texcoord.x = 1.0f;
  verts[3].texcoord.y = 0.0f;

  uint32_t indices[6] = {0, 1, 2, 0, 3, 1};

  if (!renderer_create_geometry(&state->default_geometry, 4, verts, 6, indices)) {
    DFATAL("Failed to create default geometry; application cannot continue");
    return false;
  }

  state->default_geometry.material = material_system_get_default();
  return true;
}

geometry_config geometry_system_generate_plane_config(float width,
						      float height,
						      uint32_t x_segment_count,
						      uint32_t y_segment_count,
						      float tile_x,
						      float tile_y,
						      const char* name,
						      const char* material_name) {
  if (width == 0) {
    DWARN("Width must be nonzero; defaulting to one");
    width = 1.0f;
  }
  if (height == 0) {
    DWARN("Height must be nonezero; defaulting to one");
    height = 1.0f;
  }
  if (x_segment_count < 1) {
    DWARN("x_segment_count must be a positive number; defaulting to one");
    x_segment_count = 1;
  }
  if (y_segment_count < 1) {
    DWARN("y_segment_count must be a positive number; defaultig to one");
    y_segment_count = 1;
  }
  if (tile_x == 0) {
    DWARN("tile_x must be nonezero; defaulting to one");
    tile_x = 1.0f;
  }
  if (tile_y == 0) {
    DWARN("tile_y must be nonzero; defaulting to one");
    tile_y = 1.0f;
  }

  geometry_config config;
  config.vertex_count = x_segment_count * y_segment_count * 4;
  config.vertices = dallocate(sizeof(vertex_3d) * config.vertex_count, MEMORY_TAG_ARRAY);
  config.index_count = x_segment_count * y_segment_count * 6;
  config.indices = dallocate(sizeof(uint32_t) * config.index_count, MEMORY_TAG_ARRAY);

  float seg_width = width / x_segment_count;
  float seg_height = height / y_segment_count;
  float half_width = width * 0.5f;
  float half_height = height * 0.5f;
  for (uint32_t y = 0; y < y_segment_count; ++y) {
    for (uint32_t x = 0; x < x_segment_count; ++x) {
      float min_x = (x * seg_width) - half_width;
      float min_y = (y * seg_height) - half_height;
      float max_x = min_x + seg_width;
      float max_y = min_y + seg_height;
      float min_uvx = (x / (float)x_segment_count) * tile_x;
      float min_uvy = (y / (float)y_segment_count) * tile_y;
      float max_uvx = ((x + 1) / (float)x_segment_count) * tile_x;
      float max_uvy = ((y + 1) / (float)y_segment_count) * tile_y;

      uint32_t v_offset = ((y * x_segment_count) + x) * 4;
      vertex_3d* v0 = &config.vertices[v_offset + 0];
      vertex_3d* v1 = &config.vertices[v_offset + 1];
      vertex_3d* v2 = &config.vertices[v_offset + 2];
      vertex_3d* v3 = &config.vertices[v_offset + 3];

      v0->position.x = min_x;
      v0->position.y = min_y;
      v0->texcoord.x = min_uvx;
      v0->texcoord.y = min_uvy;

      v1->position.x = max_x;
      v1->position.y = max_y;
      v1->texcoord.x = max_uvx;
      v1->texcoord.y = max_uvy;

      v2->position.x = min_x;
      v2->position.y = min_y;
      v2->texcoord.x = min_uvx;
      v2->texcoord.y = min_uvy;

      v3->position.x = max_x;
      v3->position.y = max_y;
      v3->texcoord.x = max_uvx;
      v3->texcoord.y = max_uvy;

      uint32_t i_offset = ((y * x_segment_count) + x) * 6;
      config.indices[i_offset + 0] = v_offset + 0;
      config.indices[i_offset + 1] = v_offset + 1;
      config.indices[i_offset + 2] = v_offset + 2;
      config.indices[i_offset + 3] = v_offset + 0;
      config.indices[i_offset + 4] = v_offset + 3;
      config.indices[i_offset + 5] = v_offset + 1;
    }
  }

  if (name && strlen(name) > 0) {
    strncpy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
  } else {
    strncpy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
  }

  if (material_name && strlen(material_name) > 0) {
    strncpy(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
  } else {
    strncpy(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
  }

  return config;
}
