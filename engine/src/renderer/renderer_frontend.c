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
  material* test_material;
} renderer_system_state;

static renderer_system_state* state_ptr;

bool event_on_debug_event(uint16_t code, void* sender, void* listener_inst, event_context data) {
  const char* names[3] = {
    "cobblestone",
    "paving",
    "paving2"
  };
  static int8_t choice = 2;
  const char* old_name = names[choice];
  choice++;
  choice %= 3;
  
  state_ptr->test_material->diffuse_map.texture = texture_system_acquire(names[choice], true);
  if (!state_ptr->test_material->diffuse_map.texture) {
    DWARN("event_on_debug_event no texture, using default");
    state_ptr->test_material->diffuse_map.texture = texture_system_get_default_texture();
  }
  texture_system_release(old_name);
  return true;
}

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name) {
  *memory_requirement = sizeof(renderer_system_state);
  if (state == 0) {
    return true;
  }
  state_ptr = state;

  event_register(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
  
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
    event_unregister(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
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

    mat4 model = mat4_translation((vec3){0, 0, 0});

    geometry_render_data data = {};
    data.model = model;
    if (!state_ptr->test_material) {
      state_ptr->test_material = material_system_acquire("test_material");
      if (!state_ptr->test_material) {
	DWARN("automatic material load failed, falling back to manual default");
	material_config config;
	strncpy(config.name, "test_material", MATERIAL_NAME_MAX_LENGTH);
	config.auto_release = false;
	config.diffuse_color = vec4_one();
	strncpy(config.diffuse_map_name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
	state_ptr->test_material = material_system_acquire_from_config(config);
      }
    }
    data.material = state_ptr->test_material;
    state_ptr->backend.update_object(data);

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
