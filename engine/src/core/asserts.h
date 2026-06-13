#pragma once

#include "defines.h"

#define DASSERTIONS_ENABLED

#ifdef DASSERTIONS_ENABLED
#define debugBreak() __builtin_trap()

DAPI void report_assertion_failure(const char* expression, const char* message, const char* file, int line);

#define ASSERT(expr)						\
  {								\
    if (expr) {							\
    } else {							\
      report_assertion_failure(#expr, "", __FILE__, __LINE__);	\
      debugBreak();						\
    }								\
  }

#define ASSERT_MSG(expr, message)					\
  {									\
    if (expr) {								\
    } else {								\
      report_assertion_failure(#expr, message, __FILE__, __LINE__);	\
    }									\
  }

#ifdef _DEBUG
#define ASSERT_DEBUG(expr)					\
  {								\
    if (expr) {							\
    } else {							\
      report_assertion_failure(#expr, "", __FILE__, __LINE__);	\
      debugBreak();						\
    }								\
  }
#else
#define ASSERT_DEBUG(expr)
#endif

#else
#define ASSERT(expr)
#define ASSERT_MSG(expr, message)
#define ASSERT_DEBUG(expr)
#endif
