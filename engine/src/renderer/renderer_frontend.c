#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/dmemory.h"

#include "math/dmath.h"

typedef struct renderer_system_state {
  renderer_backend backend;
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

void renderer_on_resize(uint16_t width, uint16_t height) {
  if (state_ptr) {
    state_ptr->backend.resized(&state_ptr->backend, width, height);
  } else {
    DWARN("renderer backend does not exist to accept resize: %i, %i", width, height);
  }
}

bool renderer_draw_frame(render_packet* packet) {
  if (renderer_begin_frame(packet->delta_time)) {
    mat4 projection = mat4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, 0.1f, 1000.0f);
    static float z = -1.0f;
    z -= 0.005f;
    mat4 view = mat4_translation((vec3){0, 0, z});
    state_ptr->backend.update_global_state(projection, view, vec3_zero(), vec4_one(), 0);
    bool result = renderer_end_frame(packet->delta_time);
    if (!result) {
      DERROR("renderer_end_frame() failed, application shutting down");
      return false;
    }
  }

  return true;
}
