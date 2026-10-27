#pragma once

#include "defines.h"

typedef struct platform_state {
  void* internal_state;
} platform_state;

bool pstartup(platform_state* plat_state,
			   const char* application_name,
			   int32_t x, int32_t y,
			   int32_t width, int32_t height);
void pshutdown(platform_state* plat_state);
bool ppumpmsg(platform_state* plat_state);

DAPI void* palloc(uint64_t size, bool aligned);
DAPI void pfree(void* block, bool aligned);

void pcw(const char* message, uint8_t color);
void pcwerr(const char* message, uint8_t color);

double pgat();
void psleep(uint64_t ms);
