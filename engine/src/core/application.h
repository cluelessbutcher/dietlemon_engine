#pragma once

#include "defines.h"

#include <stdbool.h>

struct game;

typedef struct application_config {
  short start_pos_x;
  short start_pos_y;
  short start_width;
  short start_height;
  char* name;
} application_config;

DAPI bool application_create(struct game* game_inst);
DAPI bool application_run();
