#include "texture_system.h"

#include "core/logger.h"
#include "core/dstring.h"
#include "core/dmemory.h"
#include "containers/hashtable.h"

#include "renderer/renderer_frontend.h"

#include "systems/resource_system.h"

#include <string.h>

typedef struct texture_system_state {
  texture_system_config config;
  texture default_texture;
  texture* registered_textures;
  hashtable registered_texture_table;
} texture_system_state;

typedef struct texture_reference {
  uint64_t reference_count;
  uint32_t handle;
  bool auto_release;
} texture_reference;

static texture_system_state* state_ptr = 0;

bool create_default_textures(texture_system_state* state);
void destroy_default_textures(texture_system_state* state);
bool load_texture(const char* texture_name, texture* t);
void destroy_texture(texture* t);

bool texture_system_initialize(uint64_t* memory_requirement, void* state, texture_system_config config) {
  if (config.max_texture_count == 0) {
    DFATAL("texture_system_initialize() - config.max_texture_count must be > 0");
    return false;
  }

  uint64_t struct_requirement = sizeof(texture_system_state);
  uint64_t array_requirement = sizeof(texture) * config.max_texture_count;
  uint64_t hashtable_requirement = sizeof(texture_reference) * config.max_texture_count;
  *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

  if (!state) {
    return true;
  }

  state_ptr = state;
  state_ptr->config = config;

  void* array_block = (uint8_t*)state + struct_requirement;
  state_ptr->registered_textures = array_block;

  void* hashtable_block = (uint8_t*)array_block + array_requirement;

  hashtable_create(sizeof(texture_reference), config.max_texture_count, hashtable_block, false, &state_ptr->registered_texture_table);

  texture_reference invalid_ref;
  invalid_ref.auto_release = false;
  invalid_ref.handle = INVALID_ID;
  invalid_ref.reference_count = 0;
  hashtable_fill(&state_ptr->registered_texture_table, &invalid_ref);

  uint32_t count = state_ptr->config.max_texture_count;
  for (uint32_t i = 0; i < count; ++i) {
    state_ptr->registered_textures[i].id = INVALID_ID;
    state_ptr->registered_textures[i].generation = INVALID_ID;
  }

  create_default_textures(state_ptr);

  return true;
}

void texture_system_shutdown(void* state) {
  if (state_ptr) {
    for (uint32_t i = 0; i < state_ptr->config.max_texture_count; ++i) {
      texture* t = &state_ptr->registered_textures[i];
      if (t->generation != INVALID_ID) {
        renderer_destroy_texture(t);
      }
    }

    destroy_default_textures(state_ptr);

    state_ptr = 0;
  }
}

texture* texture_system_acquire(const char* name, bool auto_release) {
  if (strcmp(name, DEFAULT_TEXTURE_NAME) == 0) {
    DWARN("texture_system_acquire() called for default texture; use texture_system_get_default_texture for texture 'default'");
    return &state_ptr->default_texture;
  }

  texture_reference ref;
  if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {
    if (ref.reference_count == 0) {
      ref.auto_release = auto_release;
    }

    ref.reference_count++;
    if (ref.handle == INVALID_ID) {
      uint32_t count = state_ptr->config.max_texture_count;
      texture* t = 0;
      for (uint32_t i = 0; i < count; ++i) {
        if (state_ptr->registered_textures[i].id == INVALID_ID) {
          ref.handle = i;
          t = &state_ptr->registered_textures[i];
          break;
        }
      }

      if (!t || ref.handle == INVALID_ID) {
        DFATAL("texture_system_acquire() - Texture system cannot hold anymore textures; adjust configuration to allow more");
        return 0;
      }

      if (!load_texture(name, t)) {
        DERROR("Failed to load texture: '%s'", name);
        return 0;
      }

      t->id = ref.handle;
      DTRACE("Texture '%s' does not yet exist; created, and ref_count is now %i", name, ref.reference_count);
    } else {
      DTRACE("Texture '%s' already exists; ref_count increased to %i", name, ref.reference_count);
    }

    hashtable_set(&state_ptr->registered_texture_table, name, &ref);
    return &state_ptr->registered_textures[ref.handle];
  }

  DERROR("texture_system_acquire failed to acquire texture '%s', null pointer will be returned", name);
  return 0;
}

void texture_system_release(const char* name) {
  if (strcmp(name, DEFAULT_TEXTURE_NAME) == 0) {
    return;
  }
  texture_reference ref;
  if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {
    if (ref.reference_count == 0) {
      DWARN("Tried to release non-existant texture: '%s'", name);
      return;
    }

    char name_copy[TEXTURE_NAME_MAX_LENGTH];
    strncpy(name_copy, name, TEXTURE_NAME_MAX_LENGTH);

    ref.reference_count--;
    if (ref.reference_count == 0 && ref.auto_release) {
      texture* t = &state_ptr->registered_textures[ref.handle];
      destroy_texture(t);
      ref.handle = INVALID_ID;
      ref.auto_release = false;
      DTRACE("Released texture '%s', texture unloaded because reference count = 0 and auto_release = true", name_copy);
    } else {
      DTRACE("Released texture '%s', now has a reference count of '%i' (auto_release=%s)", name_copy, ref.reference_count, ref.auto_release ? "true" : "false");
    }

    hashtable_set(&state_ptr->registered_texture_table, name_copy, &ref);
  } else {
    DERROR("texture_system_release failed to release texture '%s'", name);
  }
}

texture* texture_system_get_default_texture() {
  if (state_ptr) {
    return &state_ptr->default_texture;
  }

  DERROR("texture_system_get_default_texture() called before texture system initialization! null pointer returned");
  return 0;
}

bool create_default_textures(texture_system_state* state) {
  DTRACE("creating default texture...");
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

  strncpy(state->default_texture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
  state->default_texture.width = tex_dimension;
  state->default_texture.height = tex_dimension;
  state->default_texture.channel_count = 4;
  state->default_texture.has_transparency = false;
  renderer_create_texture(pixels, &state->default_texture);
  state->default_texture.generation = INVALID_ID;
  return true;
}

void destroy_default_textures(texture_system_state* state) {
  if (state) {
    destroy_texture(&state->default_texture);
  }
}

bool load_texture(const char* texture_name, texture* t) {
  resource img_resource;
  if (!resource_system_load(texture_name, RESOURCE_TYPE_IMAGE, &img_resource)) {
    DERROR("Failed to load image resource for texture: '%S'", texture_name);
    return false;
  }

  image_resource_data* resource_data = img_resource.data;
  
  texture temp_texture;
  temp_texture.width = resource_data->width;
  temp_texture.height = resource_data->height;
  temp_texture.channel_count = resource_data->channel_count;

  uint32_t current_generation = t->generation;
  t->generation = INVALID_ID;

  uint64_t total_size = temp_texture.width * temp_texture.height * temp_texture.channel_count;
  int has_transparency = false;
  for (uint64_t i = 0; i < total_size; i += temp_texture.channel_count) {
    uint8_t a = resource_data->pixels[i + 3];
    if (a < 255) {
      has_transparency = true;
      break;
    }
  }

  strncpy(temp_texture.name, texture_name, TEXTURE_NAME_MAX_LENGTH);
  temp_texture.generation = INVALID_ID;
  temp_texture.has_transparency = has_transparency;

  renderer_create_texture(resource_data->pixels, &temp_texture);
  texture old = *t;
  *t = temp_texture;
  
  renderer_destroy_texture(&old);

  renderer_destroy_texture(&old);

  if (current_generation == INVALID_ID) {
    t->generation = 0;
  } else {
    t->generation = current_generation + 1;
  }

  resource_system_unload(&img_resource);
  
  return true;
}


void destroy_texture(texture* t) {
  renderer_destroy_texture(t);
  memset(t->name, 0, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
  memset(t, 0, sizeof(texture));
  t->id = INVALID_ID;
  t->generation = INVALID_ID;
}
