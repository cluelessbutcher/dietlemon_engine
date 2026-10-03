#include "resource_system.h"

#include "core/logger.h"
#include "core/dstring.h"

#include "resources/loaders/text_loader.h"
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"

#include <string.h>

typedef struct resource_system_state {
  resource_system_config config;;
  resource_loader* registered_loaders;
} resource_system_state;

static resource_system_state* state_ptr = 0;

bool load(const char* name, resource_loader* loader, resource* out_resource);

bool resource_system_initialize(uint64_t* memory_requirement, void* state, resource_system_config config) {
  if (config.max_loader_count == 0) {
    DFATAL("resource_system_initialize() - failed because config.max_loader_count == 0");
    return false;
  }

  *memory_requirement = sizeof(resource_system_state) * (sizeof(resource_loader) * config.max_loader_count);
  if (!state) {
    return true;
  }

  state_ptr = state;
  state_ptr->config = config;

  void* array_block = state + sizeof(resource_system_state);
  state_ptr->registered_loaders = array_block;

  uint32_t count = config.max_loader_count;
  for (uint32_t i = 0; i < count; ++i) {
    state_ptr->registered_loaders[i].id = INVALID_ID;
  }

  resource_system_register_loader(text_resource_loader_create());
  resource_system_register_loader(binary_resource_loader_create());
  resource_system_register_loader(image_resource_loader_create());
  resource_system_register_loader(material_resource_loader_create());

  DINFO("Resource system initialize with base path '%s'", config.asset_base_path);

  return true;
}

void resource_system_shutdown(void* state) {
  if (state_ptr) {
    state_ptr = 0;
  }
}

bool resource_system_register_loader(resource_loader loader) {
  if (state_ptr) {
    uint32_t count = state_ptr->config.max_loader_count;
    for (uint32_t i = 0; i < count; ++i) {
      resource_loader* l = &state_ptr->registered_loaders[i];
      if (l->id != INVALID_ID) {
	if (l->type == loader.type) {
	  DERROR("resource_system_register_loader() - loader of type %d already exists and will not be registered", loader.type);
	  return false;
	} else if (loader.custom_type && strlen(loader.custom_type) > 0 && strcasecmp(l->custom_type, loader.custom_type) == 0) {
	  DERROR("resource_system_register_loader() - loader of custom type %s already exists and will not be registered", loader.custom_type);
	  return false;
	}
      }
    }
    for (uint32_t i = 0; i < count; ++i) {
      if (state_ptr->registered_loaders[i].id == INVALID_ID) {
	state_ptr->registered_loaders[i] = loader;
	state_ptr->registered_loaders[i].id = i;
	DTRACE("Loader registered");
	return true;
      }
    }
  }

  return false;
}

bool resource_system_load(const char* name, resource_type type, resource* out_resource) {
  if (state_ptr && type != RESOURCE_TYPE_CUSTOM) {
    uint32_t count = state_ptr->config.max_loader_count;
    for (uint32_t i = 0; i < count; ++i) {
      resource_loader* l = &state_ptr->registered_loaders[i];
      if (l->id != INVALID_ID && l->type == type) {
	return load(name, l, out_resource);
      }
    }
  }

  out_resource->loader_id = INVALID_ID;
  DERROR("resource_system_load() - no loader for type %d was found", type);
  return false;
}

bool resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource) {
  if (state_ptr && custom_type && strlen(custom_type) > 0) {
    uint32_t count = state_ptr->config.max_loader_count;
    for (uint32_t i = 0; i < count; ++i) {
      resource_loader* l = &state_ptr->registered_loaders[i];
      if (l->id != INVALID_ID && l->type == RESOURCE_TYPE_CUSTOM && strcasecmp(l->custom_type, custom_type) == 0) {
	return load(name, l, out_resource);
      }
    }
  }

  out_resource->loader_id = INVALID_ID;
  DERROR("resource_system_load_custom() - no loader for type %s was found", custom_type);
  return false;
}

void resource_system_unload(resource* resource) {
  if (state_ptr && resource) {
    if (resource->loader_id != INVALID_ID) {
      resource_loader* l = &state_ptr->registered_loaders[resource->loader_id];
      if (l->id != INVALID_ID && l->unload) {
	l->unload(l, resource);
      }
    }
  }
}

const char* resource_system_base_path() {
  if (state_ptr) {
    return state_ptr->config.asset_base_path;
  }

  DERROR("resource_system_base_path() - called before initialization, returning empty string");
  return "";
}

bool load(const char* name, resource_loader* loader, resource* out_resource) {
  if (!name || !loader || !loader->load || !out_resource) {
    out_resource->loader_id = INVALID_ID;
    return false;
  }

  out_resource->loader_id = loader->id;
  return loader->load(loader, name, out_resource);
}
