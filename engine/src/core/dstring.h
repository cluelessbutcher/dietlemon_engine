#pragma once

#include "defines.h"
#include "math/math_types.h"

DAPI int32_t string_format(char* dest, const char* format, ...);
DAPI int32_t string_format_v(char* dest, const char* format, void* va_list);
DAPI char* string_empty(char* str);
DAPI char* string_trim(char* str);
DAPI void string_mid(char* dest, const char* source, int32_t start, int32_t length);
DAPI int32_t string_index_of(char* str, char c);
DAPI bool string_to_vec4(char* str, vec4* out_vector);
DAPI bool string_to_vec3(char* str, vec3* out_vector);
DAPI bool string_to_vec2(char* str, vec2* out_vector);
DAPI bool string_to_float(char* str, float* f);
DAPI bool string_to_double(char* str, double* f);
DAPI bool string_to_int8_t(char* str, int8_t* i);
DAPI bool string_to_int16_t(char* str, int16_t* i);
DAPI bool string_to_int32_t(char* str, int32_t* i);
DAPI bool string_to_int64_t(char* str, int64_t* i);
DAPI bool string_to_uint8_t(char* str, uint8_t* u);
DAPI bool string_to_uint16_t(char* str, uint16_t* u);
DAPI bool string_to_uint32_t(char* str, uint32_t* u);
DAPI bool string_to_uint64_t(char* str, uint64_t* u);
DAPI bool string_to_bool(char* str, bool* b);
