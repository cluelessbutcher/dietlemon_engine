#include "dstring.h"

#include "core/dmemory.h"

#include <string.h>

char* strdup(const char* str) {
  long long length = strlen(str);
  char* copy = dalloc(length + 1, MEMORY_TAG_STRING);
  memcpy(copy, str, length + 1);
  return copy;
}
