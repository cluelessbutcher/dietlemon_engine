#pragma once

#include <defines.h>
#include <game_types.h>
#include <math/math_types.h>

typedef struct game_state {
  float delta_time;
  mat4 view;
  vec3 camera_position;
  vec3 camera_euler;
  bool camera_view_dirty;
} game_state;

bool game_initialize(game* game_inst);
bool game_update(game* game_inst, float delta_time);
bool game_render(game* game_inst, float delta_time);
void game_on_resize(game* game_inst, uint32_t width, uint32_t height);
