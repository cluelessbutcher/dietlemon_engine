#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

typedef enum renderer_backend_type {
  RENDERER_BACKEND_TYPE_VULKAN,
  RENDERER_BACKEND_TYPE_OPENGL,
  RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct global_uniform_object {
  mat4 projection;
  mat4 view;
  mat4 m_reserved0;
  mat4 m_reserved1;
} global_uniform_object;

typedef struct material_uniform_object {
  vec4 diffuse_color;
  vec4 v_reserved0;
  vec4 v_reserved1;
  vec4 v_reserved2;
} material_uniform_object;

typedef struct geometry_render_data {
  mat4 model;
  geometry* geometry;
} geometry_render_data;

typedef struct renderer_backend {
  uint64_t frame_number;
  
  bool (*initialize)(struct renderer_backend* backend, const char* application_name);
  void (*shutdown)(struct renderer_backend* backend);
  void (*resized)(struct renderer_backend* backend, uint16_t width, uint16_t height);
  bool (*begin_frame)(struct renderer_backend* backend, float delta_time);
  void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, int32_t mode);
  bool (*end_frame)(struct renderer_backend* backend, float delta_time);
  void (*draw_geometry)(geometry_render_data data);
  void (*create_texture)(const uint8_t* pixels, struct texture* out_texture);
  void (*destroy_texture)(struct texture* texture);
  bool (*create_material)(struct material* material);
  void (*destroy_material)(struct material* material);
  bool (*create_geometry)(geometry* geometry, uint32_t vertex_count, const vertex_3d* vertices, uint32_t index_count, const uint32_t* indices);
  void (*destroy_geometry)(geometry* geometry);
} renderer_backend;

typedef struct render_packet {
  float delta_time;
  uint32_t geometry_count;
  geometry_render_data* geometries;
} render_packet;
