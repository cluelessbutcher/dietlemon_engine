#include "material_system.h"

#include "core/logger.h"
#include "core/dstring.h"
#include "containers/hashtable.h"
#include "math/dmath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

#include "platform/filesystem.h"

#include <string.h>
#include <strings.h>

typedef struct material_system_state {
  material_system_config config;
  material default_material;
  material* registered_materials;
  hashtable registered_material_table;
} material_system_state;

typedef struct material_reference {
  uint64_t reference_count;
  uint32_t handle;
  bool auto_release;
} material_reference;

static material_system_state* state_ptr = 0;

bool create_default_material(material_system_state* state);
bool load_material(material_config config, material* m);
void destroy_material(material* m);
bool load_configuration_file(const char* path, material_config* out_config);

bool material_system_initialize(uint64_t* memory_requirement, void* state, material_system_config config) {
  if (config.max_material_count == 0) {
    DFATAL("material_system_initialize() - config.max_material_count must be > 0");
    return false;
  }

  uint64_t struct_requirement = sizeof(material_system_state);
  uint64_t array_requirement = sizeof(material) * config.max_material_count;
  uint64_t hashtable_requirement = sizeof(material_reference) * config.max_material_count;
  *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

  if (!state) {
    return true;
  }

  state_ptr = state;
  state_ptr->config = config;

  void* array_block = (uint8_t*)state + struct_requirement;
  state_ptr->registered_materials = array_block;

  void* hashtable_block = (uint8_t*)array_block + array_requirement;

  hashtable_create(sizeof(material_reference), config.max_material_count, hashtable_block, false, &state_ptr->registered_material_table);

  material_reference invalid_ref;
  invalid_ref.auto_release = false;
  invalid_ref.handle = INVALID_ID;
  invalid_ref.reference_count = 0;
  hashtable_fill(&state_ptr->registered_material_table, &invalid_ref);

  uint32_t count = state_ptr->config.max_material_count;
  for (uint32_t i = 0; i < count; ++i) {
    state_ptr->registered_materials[i].id = INVALID_ID;
    state_ptr->registered_materials[i].generation = INVALID_ID;
    state_ptr->registered_materials[i].internal_id = INVALID_ID;
  }

  if (!create_default_material(state_ptr)) {
    DFATAL("Failed to create default material application cannot continue");
    return false;
  }

  return true;
}

void material_system_shutdown(void* state) {
  material_system_state* s = (material_system_state*)state;
  if (s) {
    uint32_t count = s->config.max_material_count;
    for (uint32_t i = 0; i < count; ++i) {
      if (s->registered_materials[i].id != INVALID_ID) {
        destroy_material(&s->registered_materials[i]);
      }
    }

    destroy_material(&s->default_material);
  }

  state_ptr = 0;
}

material* material_system_acquire(const char* name) {
  material_config config;
  memset(&config, 0, sizeof(material_config));
  config.auto_release = true;
  char* format_str = "assets/materials/%s.%s";
  char full_file_path[512];

  string_format(full_file_path, format_str, name, "kmt");
  if (!load_configuration_file(full_file_path, &config)) {
    DERROR("Failed to load material file: '%s', Null pointer will be returned", full_file_path);
    return 0;
  }

  return material_system_acquire_from_config(config);
}

material* material_system_acquire_from_config(material_config config) {
  if (strcasecmp(config.name, DEFAULT_MATERIAL_NAME) == 0) {
    return &state_ptr->default_material;
  }

  material_reference ref;
  if (state_ptr && hashtable_get(&state_ptr->registered_material_table, config.name, &ref)) {
    if (ref.reference_count == 0) {
      ref.auto_release = config.auto_release;
    }
    ref.reference_count++;
    if (ref.handle == INVALID_ID) {
      uint32_t count = state_ptr->config.max_material_count;
      material* m = 0;
      for (uint32_t i = 0; i < count; ++i) {
        if (state_ptr->registered_materials[i].id == INVALID_ID) {
          ref.handle = i;
          m = &state_ptr->registered_materials[i];
          break;
        }
      }

      if (!m || ref.handle == INVALID_ID) {
        DFATAL("material_system_acquire() - Material system cannot hold anymore materials; adjust configuratino to allow more");
        return 0;
      }

      if (!load_material(config, m)) {
        DERROR("Failed to load material '%s'", config.name);
        return 0;
      }

      if (m->generation == INVALID_ID) {
        m->generation = 0;
      } else {
        m->generation++;
      }

      m->id = ref.handle;
      DTRACE("Material '%s' does not yet exist, created and ref_count is now %i", config.name, ref.reference_count);
    } else {
      DTRACE("Material '%s' already exists, ref_count increased to %i", config.name, ref.reference_count);
    }

    hashtable_set(&state_ptr->registered_material_table, config.name, &ref);
    return &state_ptr->registered_materials[ref.handle];
  }

  DERROR("material_system_acquire_from_config() - failed to acquire material '%s' so null pointer will be returned", config.name);
  return 0;
}

void material_system_release(const char* name) {
  if (strcasecmp(name, DEFAULT_MATERIAL_NAME) == 0) {
    return;
  }
  material_reference ref;
  if (state_ptr && hashtable_get(&state_ptr->registered_material_table, name, &ref)) {
    if (ref.reference_count == 0) {
      DWARN("Tried to release non-existant material: '%s'", name);
      return;
    }
    ref.reference_count--;
    if (ref.reference_count == 0 && ref.auto_release) {
      material* m = &state_ptr->registered_materials[ref.handle];
      destroy_material(m);
      ref.handle = INVALID_ID;
      ref.auto_release = false;
      DTRACE("Released material '%s', material unloaded because reference count == 0 and auto_release==true", name);
    } else {
      DTRACE("Released material '%s', now has a reference count of '%i' (auto_release = %s)", name, ref.reference_count, ref.auto_release ? "true" : "false");
    }

    hashtable_set(&state_ptr->registered_material_table, name, &ref);
  } else {
    DERROR("material_system_release failed to release materials '%s'", name);
  }
}

bool load_material(material_config config, material* m) {
  memset(m, 0, sizeof(material));
  strncpy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);
  m->diffuse_color = config.diffuse_color;
  if (strlen(config.diffuse_map_name) > 0) {
    m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    m->diffuse_map.texture = texture_system_acquire(config.diffuse_map_name, true);
    if (!m->diffuse_map.texture) {
      DWARN("Unable to load texture '%s' for material '%s' using default", config.diffuse_map_name, m->name);
      m->diffuse_map.texture = texture_system_get_default_texture();
    }
  } else {
    m->diffuse_map.use = TEXTURE_USE_UNKNOWN;
    m->diffuse_map.texture = 0;
  }

  if (!renderer_create_material(m)) {
    DERROR("Failed to acquire renderer resources for material '%s'", m->name);
    return false;
  }

  return true;
}

void destroy_material(material* m) {
  DTRACE("Destroying material '%s'...", m->name);
  if (m->diffuse_map.texture) {
    texture_system_release(m->diffuse_map.texture->name);
  }
  renderer_destroy_material(m);
  memset(m, 0, sizeof(material));
  m->id = INVALID_ID;
  m->generation = INVALID_ID;
  m->internal_id = INVALID_ID;
}

bool create_default_material(material_system_state* state) {
  memset(&state->default_material, 0, sizeof(material));
  state->default_material.id = INVALID_ID;
  state->default_material.generation = INVALID_ID;
  strncpy(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
  state->default_material.diffuse_color = vec4_one();
  state->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
  state->default_material.diffuse_map.texture = texture_system_get_default_texture();

  if (!renderer_create_material(&state->default_material)) {
    DFATAL("Failed to acquire renderer resources for default texture; Application cannot continue");
    return false;
  }

  return true;
}

bool load_configuration_file(const char* path, material_config* out_config) {
  file_handle f;
  if (!filesystem_open(path, FILE_MODE_READ, false, &f)) {
    DERROR("load_configuration_file - unable to open file for reading: '%s'", path);
    return false;
  }

  char line_buf[512] = "";
  char* p = &line_buf[0];
  uint64_t line_length = 0;
  uint32_t line_number = 1;
  while (filesystem_read_line(&f, 511, &p, &line_length)) {
    char* trimmed = string_trim(line_buf);
    line_length = strlen(trimmed);
    if (line_length < 1 || trimmed[0] == '#') {
      line_number++;
      continue;
    }

    int32_t equal_index = string_index_of(trimmed, '=');
    if (equal_index == -1) {
      DWARN("Potential formatting issue found in file '%s': '=' token not found. skipping line %u", path, line_number);
      line_number++;
      continue;
    }

    char raw_var_name[64];
    memset(raw_var_name, 0, sizeof(char) * 64);
    string_mid(raw_var_name, trimmed, 0, equal_index);
    char* trimmed_var_name = string_trim(raw_var_name);

    char raw_value[446];
    memset(raw_value, 0, sizeof(char) * 446);
    string_mid(raw_value, trimmed, equal_index + 1, -1);
    char* trimmed_value = string_trim(raw_value);

    if (strcasecmp(trimmed_var_name, "version") == 0) {

    } else if (strcasecmp(trimmed_var_name, "name") == 0) {
      strncpy(out_config->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
    } else if (strcasecmp(trimmed_var_name, "diffuse_map_name") == 0) {
      strncpy(out_config->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
    } else if (strcasecmp(trimmed_var_name, "diffuse_color") == 0) {
      if (!string_to_vec4(trimmed_value, &out_config->diffuse_color)) {
        DWARN("Error parsing diffuse_color in file '%s'; using the defualt white instead", path);
        out_config->diffuse_color = vec4_one();
      }
    }

    memset(line_buf, 0, sizeof(char) * 512);
    line_number++;
  }

  filesystem_close(&f);
  return true;
}
