#pragma once

#include "resources/resource_types.h"

typedef struct resource_system_config {
  uint32_t max_loader_count;
  char* asset_base_path;
} resource_system_config;

typedef struct resource_loader {
  uint32_t id;
  resource_type type;
  const char* custom_type;
  const char* type_path;
  bool (*load)(struct resource_loader* self, const char* name, resource* out_resource);
  void (*unload)(struct resource_loader* self, resource* resource);
} resource_loader;

bool resource_system_initialize(uint64_t* memory_requirement, void* state, resource_system_config config);
void resource_system_shutdown(void* state);

DAPI bool resource_system_register_loader(resource_loader loader);
DAPI bool resource_system_load(const char* name, resource_type type, resource* out_resource);
DAPI bool resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource);
DAPI void resource_system_unload(resource* resource);

DAPI const char* resource_system_base_path();
