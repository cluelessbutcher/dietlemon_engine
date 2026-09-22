#include "game.h"

#include <core/logger.h>
#include <core/dmemory.h>
#include <core/input.h>
#include <math/dmath.h>
#include <renderer/renderer_frontend.h>

void recalculate_view_matrix(game_state* state) {
  if (state->camera_view_dirty) {
    mat4 rotation = mat4_euler_xyz(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
    mat4 translation = mat4_translation(state->camera_position);
    state->view = mat4_mul(rotation, translation);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = false;
  }
} 

void camera_yaw(game_state* state, float amount) {
  state->camera_euler.y += amount;
  state->camera_view_dirty = true;
}

void camera_pitch(game_state* state, float amount) {
  state->camera_euler.x += amount;
  float limit = deg_to_rad(89.0f);
  state->camera_euler.x = DCLAMP(state->camera_euler.x, -limit, limit);
  state->camera_view_dirty = true;
}

bool game_initialize(game* game_inst) {
  DDEBUG("game_initialize() called");
  
  game_state* state = (game_state*)game_inst->state;
  state->camera_position = (vec3){0, 0, 30.0f};
  state->camera_euler = vec3_zero();
  state->view = mat4_translation(state->camera_position);
  state->view = mat4_inverse(state->view);
  state->camera_view_dirty = true;
  return true;
}

bool game_update(game* game_inst, float delta_time) {
  static uint64_t alloc_count = 0;
  uint64_t prev_alloc_count = alloc_count;
  alloc_count = get_memory_alloc_count();
  if (input_is_key_up('M') && input_was_key_down('M')) {
    DDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
  }
  
  game_state* state = (game_state*)game_inst->state;
  
  if (input_is_key_down('A') || input_is_key_down(KEY_LEFT)) {
    camera_yaw(state, 1.0f * delta_time);
  }
  if (input_is_key_down('D') || input_is_key_down(KEY_RIGHT)) {
    camera_yaw(state, -1.0f * delta_time);
  }
  if (input_is_key_down(KEY_UP)) {
    camera_pitch(state, 1.0f * delta_time);
  }
  if (input_is_key_down(KEY_DOWN)) {
    camera_pitch(state, -1.0f * delta_time);
  }
  
  float temp_move_speed = 50.0f;
  vec3 velocity = vec3_zero();
  
  if (input_is_key_down('W')) {
    vec3 forward = mat4_forward(state->view);
    velocity = vec3_add(velocity, forward);
  }
  if (input_is_key_down('S')) {
    vec3 backward = mat4_backward(state->view);
    velocity = vec3_add(velocity, backward);
  }
  if (input_is_key_down('Q')) {
    vec3 left = mat4_left(state->view);
    velocity = vec3_add(velocity, left);
  }
  if (input_is_key_down('E')) {
    vec3 right = mat4_right(state->view);
    velocity = vec3_add(velocity, right);
  }
  if (input_is_key_down(KEY_SPACE)) {
    velocity.y += 1.0f;
  }
  if (input_is_key_down('X')) {
    velocity.y -= 1.0f;
  }
  
  vec3 z = vec3_zero();
  if (!vec3_compare(z, velocity, 0.0002f)) {
    vec3_normalize(&velocity);
    state->camera_position.x += velocity.x * temp_move_speed * delta_time;
    state->camera_position.y += velocity.y * temp_move_speed * delta_time;
    state->camera_position.z += velocity.z * temp_move_speed * delta_time;
    state->camera_view_dirty = true;
  }
  
  recalculate_view_matrix(state);
  
  renderer_set_view(state->view);
  
  return true;
}

bool game_render(game* game_inst, float delta_time) {
  return true;
}

void game_on_resize(game* game_inst, uint32_t width, uint32_t height) {

}
