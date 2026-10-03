#include "material_loader.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/dstring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/dmath.h"

#include "platform/filesystem.h"

bool material_loader_load(struct resource_loader* self, const char* name, resource* out_resource) {
  if (!self || !name || !out_resource) {
    return false;
  }

  char* format_str = "%s/%s/%s%s";
  char full_file_path[512];
  string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".kmt");

  out_resource->full_path = strdup(full_file_path);

  file_handle f;
  if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &f)) {
    DERROR("material_loader_load() - unable to open material file for reading: %s", full_file_path);
    return false;
  }

  material_config* resource_data = dallocate(sizeof(material_config), MEMORY_TAG_METERIAL_INSTANCE);
  resource_data->auto_release = true;
  resource_data->diffuse_color = vec4_one();
  resource_data->diffuse_map_name[0] = 0;
  strncpy(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

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
      DWARN("Potential formatting issue found in file '%s': '=' token was not found; skipping line %ui", full_file_path, line_number);
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
      strncpy(resource_data->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
    } else if (strcasecmp(trimmed_var_name, "diffuse_color") == 0) {
      if (!string_to_vec4(trimmed_value, &resource_data->diffuse_color)) {
	DWARN("Error parsin diffuse_color in file '%s', using default of white instead", full_file_path);
      }
    }

    memset(line_buf, 0, sizeof(char) * 512);
    line_number++;
  }

  filesystem_close(&f);

  out_resource->data = resource_data;
  out_resource->data_size = sizeof(material_config);
  out_resource->name = name;

  return true;
}

void material_loader_unload(struct resource_loader* self, resource* resource) {
  if (!self || !resource) {
    DWARN("material_loader_unload() - called with nullptr for self or resource");
    return;
  }

  uint32_t path_length = strlen(resource->full_path);
  if (path_length) {
    dfree(resource->full_path, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
  }

  if (resource->data) {
    dfree(resource->data, resource->data_size, MEMORY_TAG_METERIAL_INSTANCE);
    resource->data = 0;
    resource->data_size = 0;
    resource->loader_id = INVALID_ID;
  }
}

resource_loader material_resource_loader_create() {
  resource_loader loader;
  loader.type = RESOURCE_TYPE_MATERIAL;
  loader.custom_type = 0;
  loader.load = material_loader_load;
  loader.unload = material_loader_unload;
  loader.type_path = "material";

  return loader;
}
