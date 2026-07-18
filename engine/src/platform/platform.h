#pragma once

#include "defines.h"

typedef struct platform_state {
    void* internal_state;
} platform_state;

DAPI bool platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y, int32_t width, int32_t height);
DAPI void platform_shutdown(platform_state* plat_state);
DAPI bool platform_pump_messages(platform_state* plat_state);

void* platform_allocate(uint64_t size, bool aligned);
void platform_free(void* block, bool aligned);

void platform_console_write(const char* message, uint8_t color);
void platform_console_write_error(const char* message, uint8_t color);

double platform_get_absolute_time();

void platform_sleep(uint64_t ms);