#include "renderer_frontend.h"
#include "renderer_backend.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/dstring.h"
#include "core/event.h"

#include "math/dmath.h"

#include "resources/resource_types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#include <string.h>

typedef struct renderer_system_state {
  renderer_backend backend;
  mat4 projection;
  mat4 view;
  float near_clip;
  float far_clip;
  texture default_texture;
  texture test_diffuse;
} renderer_system_state;

static renderer_system_state* state_ptr;

void create_texture(texture* t) {
  memset(t, 0, sizeof(texture));
  t->generation = INVALID_ID;
}

bool load_texture(const char* texture_name, texture* t) {
  char* format_str = "assets/textures/%s.%s";
  const int32_t required_channel_count = 4;
  stbi_set_flip_vertically_on_load(true);
  char full_file_path[512];
  
  string_format(full_file_path, format_str, texture_name, "png");
  
  texture temp_texture;
  
  uint8_t* data = stbi_load(full_file_path,
                            (int32_t*)&temp_texture.width,
                            (int32_t*)&temp_texture.height,
                            (int32_t*)&temp_texture.channel_count,
                            required_channel_count);
  
  temp_texture.channel_count = required_channel_count;
  
  if (data) {
    uint32_t current_generation = t->generation;
    t->generation = INVALID_ID;
    
    uint64_t total_size = temp_texture.width * temp_texture.height * required_channel_count;
    int has_transparency = false;
    for (uint64_t i = 0; i < total_size; i += required_channel_count) {
      uint8_t a = data[i + 3];
      if (a < 255) {
        has_transparency = true;
        break;
      }
    }
    
    if (stbi_failure_reason()) {
      DWARN("load_texture() failed to load file '%s': %s", full_file_path, stbi_failure_reason());
    }
    
    renderer_create_texture(texture_name, true, temp_texture.width, temp_texture.height, temp_texture.channel_count, data, has_transparency, &temp_texture);
    texture old = *t;
    *t = temp_texture;
    renderer_destroy_texture(&old);
    
    if (current_generation == INVALID_ID) {
      t->generation = 0;
    } else {
      t->generation = current_generation + 1;
    }
    
    stbi_image_free(data);
    return true;
  } else {
    if (stbi_failure_reason()) {
      DWARN("load_texture() failed to load file '%s': %s", full_file_path, stbi_failure_reason());
    }
    
    return false;
  }
}

bool event_on_debug_event(uint16_t code, void* sender, void* listener_inst, event_context data) {
  const char* names[3] = {
    "cobblestone",
    "paving",
    "paving2"
  };
  static int8_t choice = 2;
  choice++;
  choice %= 3;
  
  load_texture(names[choice], &state_ptr->test_diffuse);
  return true;
}

bool renderer_system_initialize(uint64_t* memory_requirement, void* state, const char* application_name) {
  *memory_requirement = sizeof(renderer_system_state);
  if (state == 0) {
    return true;
  }
  state_ptr = state;

  event_register(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
  state_ptr->backend.default_diffuse = &state_ptr->default_texture;
  
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

  DTRACE("Creating default texture...");
  const uint32_t tex_dimension = 256;
  const uint32_t channels = 4;
  const uint32_t pixel_count = tex_dimension * tex_dimension;
  uint8_t pixels[pixel_count * channels];
  memset(pixels, 255, sizeof(uint8_t) * pixel_count * channels);

  for (uint64_t row = 0; row < tex_dimension; ++row) {
    for (uint64_t col = 0; col < tex_dimension; ++col) {
      uint64_t index = (row * tex_dimension) + col;
      uint64_t index_bpp = index * channels;
      if (row % 2) {
        if (col % 2) {
          pixels[index_bpp + 0] = 0;
          pixels[index_bpp + 1] = 0;
        }
      } else {
        if (!(col % 2)) {
          pixels[index_bpp + 0] = 0;
          pixels[index_bpp + 1] = 0;
        }
      }
    }
  }

  renderer_create_texture(
                          "default",
                          false,
                          tex_dimension,
                          tex_dimension,
                          4,
                          pixels,
                          false,
                          &state_ptr->default_texture);

  state_ptr->default_texture.generation = INVALID_ID;
  create_texture(&state_ptr->test_diffuse);
  
  return true;
}

void renderer_system_shutdown(void* state) {
  if (state_ptr) {
    event_unregister(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
    renderer_destroy_texture(&state_ptr->default_texture);
    renderer_destroy_texture(&state_ptr->test_diffuse);
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
    data.object_id = 0;
    data.model = model;
    data.textures[0] = &state_ptr->test_diffuse;
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

void renderer_create_texture(const char* name, bool auto_release, int32_t width, int32_t height, int32_t channel_count, const uint8_t* pixels, bool has_transparency, struct texture* out_texture) {
  state_ptr->backend.create_texture(name, auto_release, width, height, channel_count, pixels, has_transparency, out_texture);
}

void renderer_destroy_texture(struct texture* texture) {
  state_ptr->backend.destroy_texture(texture);
}
