#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/dmemory.h"

#include "game_types.h"

extern bool create_game(game* out_game);

int main(void) {
    game game_inst;

    initialize_memory();
    
    if (!create_game(&game_inst)) {
        DFATAL("Could not create game");
        return -1;
    }

    if (!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize) {
        DFATAL("The game's function pointers are not assigned");
        return -2;
    }

    if (!application_create(&game_inst)) {
        DINFO("Application failed to create");
        return 1;
    }

    if (!application_run()) {
        DINFO("Application did not shutdown gracefully");
        return 2;
    }

    shutdown_memory();
    
    return 0;
}
