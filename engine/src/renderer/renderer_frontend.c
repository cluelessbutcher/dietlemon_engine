#include "renderer_frontend.h"
#include "renderer_backend.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/dstring.h"
#include "core/event.h"

#include "math/dmath.h"

#include "resources/resource_types.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

#include <string.h>

typedef struct renderer_system_state {
  renderer_backend backend;
  mat4 projection;
  mat4 view;
  float near_clip;
  float far_clip;
} renderer_system_state;

static renderer_system_state* state_ptr;

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name) {
  *memory_requirement = sizeof(renderer_system_state);
  if (state == 0) {
    return true;
  }
  state_ptr = state;
  
  renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
  state_ptr->backend.frame_number = 0;

  if (!state_ptr->backend.initialize(&state_ptr->backend, application_name)) {
    DFATAL("Renderer backend failed to initialize. Shutting down.");
    return false;
  }

  state_ptr->near_clip = 0.1f;
  state_ptr->far_clip = 1000.0f;
  state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);
  state_ptr->view = mat4_translation((vec3){0, 0, -30.0f});
  state_ptr->view = mat4_inverse(state_ptr->view);
  return true;
}

void renderer_system_shutdown(void* state) {
  if (state_ptr) {
    state_ptr->backend.shutdown(&state_ptr->backend);
  }
  state_ptr = 0;
}

bool renderer_begin_frame(float delta_time) {
  if (!state_ptr) {
    return false;
  }
  return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

bool renderer_end_frame(float delta_time) {
  if (!state_ptr) {
    return false;
  }
  bool result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
  state_ptr->backend.frame_number++;
  return result;
}

void renderer_on_resized(uint16_t width, uint16_t height) {
  if (state_ptr) {
    state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), width / (float)height, state_ptr->near_clip, state_ptr->far_clip);
    state_ptr->backend.resized(&state_ptr->backend, width, height);
  } else {
    DWARN("renderer backend does not exist to accept resize: %i %i", width, height);
  }
}

bool renderer_draw_frame(render_packet* packet) {
  if (renderer_begin_frame(packet->delta_time)) {
    state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

    uint32_t count = packet->geometry_count;
    for (uint32_t i = 0; i < count; ++i) {
      state_ptr->backend.draw_geometry(packet->geometries[i]);
    }
    
    bool result = renderer_end_frame(packet->delta_time);
    if (!result) {
      DERROR("renderer_end_frame failed. Application shutting down...");
      return false;
    }
  }
  return true;
}

void renderer_set_view(mat4 view) {
  state_ptr->view = view;
}

void renderer_create_texture(const uint8_t* pixels, struct texture* texture) {
  state_ptr->backend.create_texture(pixels, texture);
}

void renderer_destroy_texture(struct texture* texture) {
  state_ptr->backend.destroy_texture(texture);
}

bool renderer_create_material(struct material* material) {
  return state_ptr->backend.create_material(material);
}

void renderer_destroy_material(struct material* material) {
  state_ptr->backend.destroy_material(material);
}

bool renderer_create_geometry(geometry* geometry, uint32_t vertex_count, const vertex_3d* vertices, uint32_t index_count, const uint32_t* indices) {
  return state_ptr->backend.create_geometry(geometry, vertex_count, vertices, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry) {
  state_ptr->backend.destroy_geometry(geometry);
}
