#pragma once

#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

#if DEXPORT
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
