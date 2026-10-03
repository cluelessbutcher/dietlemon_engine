#include "binary_loader.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/dstring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/dmath.h"

#include "platform/filesystem.h"

bool binary_loader_load(struct resource_loader* self, const char* name, resource* out_resource) {
  if (!self || !name || !out_resource) {
    return false;
  }

  char* format_str = "%s/%s/%s%s";
  char full_file_path[512];
  string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, "");

  out_resource->full_path = strdup(full_file_path);

  file_handle f;
  if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &f)) {
    DERROR("binary_loader_load() - unable to open file for binary reading: '%s'", full_file_path);
    return false;
  }

  uint64_t file_size = 0;
  if (!filesystem_size(&f, &file_size)) {
    DERROR("Unable to binary read file: '%s'", full_file_path);
    filesystem_close(&f);
    return false;
  }

  uint8_t* resource_data = dallocate(sizeof(uint8_t) * file_size, MEMORY_TAG_ARRAY);
  uint64_t read_size = 0;
  if (!filesystem_read_all_bytes(&f, resource_data, &read_size)) {
    DERROR("Unable to binary read file: %s", full_file_path);
    filesystem_close(&f);
    return false;
  }

  filesystem_close(&f);

  out_resource->data= resource_data;
  out_resource->data_size = read_size;
  out_resource->name = name;

  return true;
}

void binary_loader_unload(struct resource_loader* self, resource* resource) {
  if (!self || !resource) {
    DWARN("binary_loader_unload() - called with nullptr for self or resource");
    return;
  }

  uint32_t path_length = strlen(resource->full_path);
  if (path_length) {
    dfree(resource->full_path, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
  }

  if (resource->data) {
    dfree(resource->data, resource->data_size, MEMORY_TAG_ARRAY);
    resource->data = 0;
    resource->data_size = 0;
    resource->loader_id = INVALID_ID;
  }
}

resource_loader binary_resource_loader_create() {
  resource_loader loader;
  loader.type = RESOURCE_TYPE_BINARY;
  loader.custom_type = 0;
  loader.load = binary_loader_load;
  loader.unload = binary_loader_unload;
  loader.type_path = "";

  return loader;
}
