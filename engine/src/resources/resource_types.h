#pragma once

#include "math/math_types.h"

#define TEXTURE_NAME_MAX_LENGTH 512

typedef struct texture {
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint8_t channel_count;
  bool has_transparency;
  uint32_t generation;
  char name[TEXTURE_NAME_MAX_LENGTH];
  void* internal_data;
} texture;

typedef enum texture_use {
  TEXTURE_USE_UNKNOWN = 0x00,
  TEXTURE_USE_MAP_DIFFUSE = 0x01,
} texture_use;

typedef struct texture_map {
  texture* texture;
  texture_use use;
} texture_map;

#define MATERIAL_NAME_MAX_LENGTH 256
typedef struct material {
  uint32_t id;
  uint32_t generation;
  uint32_t internal_id;
  char name[MATERIAL_NAME_MAX_LENGTH];
  vec4 diffuse_color;
  texture_map diffuse_map;
} material;
