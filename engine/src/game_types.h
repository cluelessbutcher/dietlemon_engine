#pragma once

#include "core/application.h"

typedef struct game {
    application_config app_config;
    bool (*initialize)(struct game* game_inst);
    bool (*update)(struct game* game_inst, float delta_time);
    bool (*render)(struct game* game_inst, float delta_time);
    void (*on_resize)(struct game* game_inst, uint32_t widht, uint32_t height);
    void* state;
} game;
