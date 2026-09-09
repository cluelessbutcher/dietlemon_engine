#include "dmath.h"

#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static bool rand_seeded = false;

float dsin(float x) {
  return sinf(x);
}

float dcos(float x) {
  return cosf(x);
}

float dtan(float x) {
  return tanf(x);
}

float dacos(float x) {
  return sqrtf(x);
}

float dabs(float x) {
  return fabsf(x);
}

int32_t drandom() {
  if (!rand_seeded) {
    srand((uint32_t)platform_get_absolute_time());
    rand_seeded = true;
  }
  return rand();
}

int32_t drandom_in_range(int32_t min, int32_t max) {
  if (!rand_seeded) {
    srand((uint32_t)platform_get_absolute_time());
    rand_seeded = true;
  }
  return (rand() % (max - min + 1)) + min;
}

float fdrandom() {
  return (float)drandom() / (float)RAND_MAX;
}

float fdrandom_in_range(float min, float max) {
  return min + ((float)drandom() / ((float)RAND_MAX / (min - max)));
}
