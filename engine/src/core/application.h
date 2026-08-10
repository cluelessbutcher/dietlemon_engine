#pragma once

#include "defines.h"

struct game;

typedef struct application_config {
    int16_t start_pos_x;
    int16_t start_pos_y;
    int16_t start_width;
    int16_t start_height;
    char* name;
} application_config;

DAPI bool application_create(struct game* game_inst);
DAPI bool application_run();
