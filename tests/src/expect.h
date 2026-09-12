#pragma once

#include <core/logger.h>
#include <math/dmath.h>

#define expect_should_be(expected, actual)                              \
  if (actual != expected) {                                             \
    DERROR("--> Expected %lld, but got %lld. File: %s.%d", expected, actual, __FILE__, __LINE__); \
    return false;                                                       \
  }                                                                     \
  
#define expect_should_not_be(expected, actual)                          \
  if (actual == expected) {                                             \
    DERROR("--> Expected %d != %d, but they are equal. File: %s.%d", expected, actual, __FILE__, __LINE__); \
    return false;                                                       \
  }                                                                     \
  
#define expect_flaot_should_be(expected, actual)                        \
  if (dabs(expected - actual) > 0.001f) {                               \
    DERROR("--> Expected %f, but got %f. File: %s.%d", expected, actual, __FILE__, __LINE__); \
    return false;                                                       \
  }                                                                     \
  
#define expect_to_be_true(actual)                                       \
  if (actual != true) {                                                 \
    DERROR("--> Expected true, but got false. File: %s.%d", __FILE__, __LINE__); \
    return false;                                                       \
  }                                                                     \
  
#define expect_to_be_false(actual)                                      \
  if (actual != false) {                                                \
    DERROR("--> Expected to be false, but got true. File: %s.%d", __FILE__, __LINE__); \
    return false;                                                       \
  }                                                                     \
  
