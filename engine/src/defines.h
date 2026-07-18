#pragma once

#include <stdint.h>
#include <stdbool.h>

#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

STATIC_ASSERT(sizeof(uint8_t) == 1, "Expected uint8_t to be 1 byte");
STATIC_ASSERT(sizeof(uint16_t) == 2, "Expected uint16_t to be 2 bytes");
STATIC_ASSERT(sizeof(uint32_t) == 4, "Expected uint32_t to be 4 bytes");
STATIC_ASSERT(sizeof(uint64_t) == 8, "Expected uint64_t to be 8 bytes");

STATIC_ASSERT(sizeof(int8_t) == 1, "Expected uint8_t to be 1 byte");
STATIC_ASSERT(sizeof(int16_t) == 2, "Expected uint16_t to be 2 bytes");
STATIC_ASSERT(sizeof(int32_t) == 4, "Expected uint32_t to be 4 bytes");
STATIC_ASSERT(sizeof(int64_t) == 8, "Expected uint64_t to be 8 bytes");

STATIC_ASSERT(sizeof(float) == 4, "Expected float to be 4 bytes");
STATIC_ASSERT(sizeof(double) == 8, "Expected double to be 8 bytes");

#ifdef DEXPORT
#define DAPI __attribute__((visibility("default")))
#else
#define DAPI
#endif