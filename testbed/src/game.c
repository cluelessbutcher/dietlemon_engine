#include "game.h"

#include <core/logger.h>

bool game_initialize(game* game_inst) {
  DEBUG("game_initialize() called");

  return true;
}

bool game_update(game* game_inst, float delta_time) {
  return true;
}

bool game_render(game* game_inst, float delta_time) {
  return true;
}

void game_on_resize(game* game_inst, int width, int height) {}
