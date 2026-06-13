#pragma once

#include "game_types.h"

#include "core/application.h"
#include "core/logger.h"
#include "core/dmemory.h"

extern bool create_game(game* out_game);

int main(void) {
  initialize_memory();
  
  game game_inst;
  if (!create_game(&game_inst)) {
    FATAL("Failed to create game");
    return -1;
  }

  if (!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize) {
    FATAL("The game's function pointers are not assigned");
    return -2;
  }
  
  if (!application_create(&game_inst)) {
    INFO("Failed to create application");
    return 1;
  }
  
  if (!application_run()) {
    INFO("Application didn't shutdown currectly");
    return 2;
  }

  shutdown_memory();
  
  return 0;
}
