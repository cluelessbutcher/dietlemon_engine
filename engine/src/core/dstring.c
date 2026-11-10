#include "core/dstring.h"

#include "core/dmemory.h"

#include <string.h>

char* dstrdup(const char* str) {
    uint64_t length = strlen(str);
    char* copy = dalloc(length + 1, MEMORY_TAG_STRING);
    memcpy(copy, str, length + 1);
    return copy;
}
