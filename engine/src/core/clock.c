#include "clock.h"

#include "platform/platform.h"

void clock_update(clock* clock) {
    if (clock->start_time != 0) {
        clock->elapsed = pgat() - clock->start_time;
    }
}

void clock_start(clock* clock) {
    clock->start_time = pgat();
    clock->elapsed = 0;
}

void clock_stop(clock* clock) {
    clock->start_time = 0;
}