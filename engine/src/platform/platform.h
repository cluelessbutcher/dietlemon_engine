#pragma once

#include "defines.h"

#include <stdbool.h>

typedef struct platform_state {
  void* internal_state;
} platform_state;

bool pstartup(platform_state* plat_state, const char* application_name, int x, int y, int width, int height);
void pshutdown(platform_state* plat_state);
bool ppump_messages(platform_state* plat_state);

void* palloc(long long size, bool aligned);
void pfree(void* block, bool aligned);

void pconsole_write(const char* message, char color);
void pconsole_write_error(const char* message, char color);

double pget_absolute_time();
void psleep(long long ms);
