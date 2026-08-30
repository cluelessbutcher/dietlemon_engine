#pragma once

#include "defines.h"

typedef struct clock {
    double start_time;
    double elapsed;
} clock;

void clock_update(clock* clock);
void clock_start(clock* clock);
void clock_stop(clock* clock);
