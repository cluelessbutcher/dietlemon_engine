#pragma

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

STATIC_ASSERT(sizeof(int8_t) == 1, "Expected int8_t to be 1 byte");
STATIC_ASSERT(sizeof(int16_t) == 2, "Expected int16_t to be 2 bytes");
STATIC_ASSERT(sizeof(int32_t) == 4, "Expected int32_t to be 4 bytes");
STATIC_ASSERT(sizeof(int64_t) == 8, "Expected int64_t to be 8 bytes");

STATIC_ASSERT(sizeof(float) == 4, "Expected float to be 4 bytes");
STATIC_ASSERT(sizeof(double) == 8, "Expected double to be 8 bytes");

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define DPLATFORM_WINDOW 1
#ifndef _WIN64
#error "64-bit is required on windows"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
#define DPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define DPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
#define DPLATFORM_UNIX 1
#elif defined(_POXIS_VERSION)
#define DPLATFORM_POXIS 1
#elif __APPLE__
#define DPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
#define DPLATFORM_IOS 1
#define DPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define DPLATFORM_IOS 1
#elif TARGET_OS_MAX
#else
#error "Unknown Apple Platform"
#endif
#else
#error "Unknown Platform"
#endif

#ifdef DEXPORT
#ifdef _MSC_VER
#define DAPI __declspec(dllexport)
#else
#define DAPI __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define DAPI __declspec(dllimport)
#else
#define DAPI
#endif
#endif
