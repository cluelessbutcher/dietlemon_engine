#include "core/dstring.h"
#include "core/dmemory.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

int32_t string_format(char* dest, const char* format, ...) {
  if (dest) {
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, format);
    int32_t written = string_format_v(dest, format, arg_ptr);
    va_end(arg_ptr);
    return written;
  }
  return -1;
}

int32_t string_format_v(char* dest, const char* format, void* va_listp) {
  if (dest) {
    char buffer[32000];
    int32_t written = vsnprintf(buffer, 32000, format, va_listp);
    buffer[written] = 0;
    memcpy(dest, buffer, written + 1); 
    return written;
  }
  return -1;
}
