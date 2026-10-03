#pragma once

#include "math/math_types.h"

typedef enum resource_type {
  RESOURCE_TYPE_TEXT,
  RESOURCE_TYPE_BINARY,
  RESOURCE_TYPE_IMAGE,
  RESOURCE_TYPE_MATERIAL,
  RESOURCE_TYPE_STATIC_MESH,
  RESOURCE_TYPE_CUSTOM
} resource_type;

typedef struct resource {
  uint32_t loader_id;
  const char* name;
  char* full_path;
  uint64_t data_size;
  void* data;
} resource;

typedef struct image_resource_data {
  uint8_t channel_count;
  uint32_t width;
  uint32_t height;
  uint8_t* pixels;
} image_resource_data;

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

typedef struct material_config {
  char name[MATERIAL_NAME_MAX_LENGTH];
  bool auto_release;
  vec4 diffuse_color;
  char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config;

typedef struct material {
  uint32_t id;
  uint32_t generation;
  uint32_t internal_id;
  char name[MATERIAL_NAME_MAX_LENGTH];
  vec4 diffuse_color;
  texture_map diffuse_map;
} material;

#define GEOMETRY_NAME_MAX_LENGTH 256

typedef struct geometry {
  uint32_t id;
  uint32_t internal_id;
  uint32_t generation;
  char name[GEOMETRY_NAME_MAX_LENGTH];
  material* material;
} geometry;
