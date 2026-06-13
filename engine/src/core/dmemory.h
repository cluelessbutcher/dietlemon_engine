#pragma once

#include "defines.h"

typedef enum memory_tag {
  MEMORY_TAG_UNKNOWN,
  MEMORY_TAG_ARRAY,
  MEMORY_TAG_DARRAY,
  MEMORY_TAG_DICT,
  MEMORY_TAG_RING_QUEUE,
  MEMORY_TAG_BST,
  MEMORY_TAG_STRING,
  MEMORY_TAG_APPLICATION,
  MEMORY_TAG_JOB,
  MEMORY_TAG_TEXTURE,
  MEMORY_TAG_MATERIAL_INSTANCE,
  MEMORY_TAG_RENDERER,
  MEMORY_TAG_GAME,
  MEMORY_TAG_TRANSFORM,
  MEMORY_TAG_ENTITY,
  MEMORY_TAG_ENTITIY_MODE,
  MEMORY_TAG_SCENE,

  MEMORY_TAG_MAX_TAGS
} memory_tag;

DAPI void initialize_memory();
DAPI void shutdown_memory();

DAPI void* dalloc(long long size, memory_tag tag);
DAPI void dfree(void* block, long long size, memory_tag tag);
DAPI char* get_memory_usage_str();
