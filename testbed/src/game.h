#pragma once

#include <defines.h>
#include <game_types.h>

#include <stdbool.h>

typedef struct game_state {
  float delta_time;
} game_state;

bool game_initialize(game* game_inst);
bool game_update(game* game_inst, float delta_time);
bool game_render(game* game_inst, float delta_time);
void game_on_resize(game* game_inst, int width, int height);
