#pragma once

#include "defines.h"

bool platform_system_startup(uint64_t* memory_requirement, void* state, const char* application_name, int32_t x, int32_t y, int32_t width, int32_t height);
void platform_system_shutdown(void* plat_state);
bool platform_pump_messages();

void* platform_allocate(uint64_t size, bool aligned);
void platform_free(void* block, bool aligned);

void platform_console_write(const char* message, uint8_t color);
void platform_console_write_error(const char* message, uint8_t color);

double platform_get_absolute_time();

void platform_sleep(uint64_t ms);
