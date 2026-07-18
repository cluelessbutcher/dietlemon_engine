#pragma once

#include "defines.h"

#define DASSERTIONS_ENABLED

#ifdef DASSERTIONS_ENABLED
void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line);
#define DASSERT(expr)                                                   \
    {                                                                   \
        if (expr) {                                                     \
        } else {                                                        \
            report_assertion_failure(#expr, "", __FILE__, __LINE__);    \
            __builtin_trap();                                           \
        }                                                               \
    }
#define DASSERT_MSG(expr, message)                                          \
    {                                                                       \
        if (expr) {                                                         \
        } else {                                                            \
            report_assertion_failure(#expr, message, __FILE__, __LINE__);   \
            __builtin_trap();                                               \
        }                                                                   \
    }
#ifdef _DEBUG
#define DASSERT_DEBUG(expr)                                             \
    {                                                                   \
        if (expr) {                                                     \
        } else {                                                        \
            report_assertion_failure(#expr, "", __FILE__, __LINE__);    \
            __builtin_trap();                                           \
        }                                                               \
    }
#else
#define DASSERT_DEBUG(expr)
#endif
#else
#define DASSERT(expr)
#define DASSERT_MSG(expr, message)
#define DASSERT_DEBUG(expr)                                      
#endif