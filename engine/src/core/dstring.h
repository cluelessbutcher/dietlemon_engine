#pragma once

#include "defines.h"

DAPI int32_t string_format(char* dest, const char* format, ...);
DAPI int32_t string_format_v(char* dest, const char* format, void* va_list);
