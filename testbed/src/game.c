#include "game.h"

#include <core/logger.h>
#include <core/dmemory.h>
#include <core/input.h>

bool game_initialize(game* game_inst) {
    DDEBUG("game_initialize() called");
    return true;
}

bool game_update(game* game_inst, float delta_time) {
  static uint64_t alloc_count = 0;
  uint64_t prev_alloc_count = alloc_count;
  alloc_count = get_memory_alloc_count();
  if (input_is_key_up('M') && input_was_key_down('M')) {
    DDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
  }
  
  return true;
}

bool game_render(game* game_inst, float delta_time) {
  return true;
}

void game_on_resize(game* game_inst, uint32_t width, uint32_t height) {

}
