#include "core/dstring.h"
#include "core/dmemory.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

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

char* string_trim(char* str) {
  while (isspace((unsigned char)*str)) {
    str++;
  }
  if (*str) {
    char* p = str;
    while (*p) {
      p++;
    }
    while (isspace((unsigned char)*(--p))) {
      p[1] = '\0';
    }
  }
  return str;
}

void string_mid(char* dest, const char* source, int32_t start, int32_t length) {
  if (length == 0) {
    return;
  }
  uint64_t src_length = strlen(source);
  if ((uint64_t)start >= src_length) {
    dest[0] = 0;
    return;
  }
  if (length > 0) {
    uint64_t j = 0;
    for (uint64_t i = start; j < (uint64_t)length && source[i]; ++i, ++j) {
      dest[j] = source[i];
    }
    dest[j] = 0;
  } else {
    uint64_t j = 0;
    for (uint64_t i = start; source[i]; ++i, ++j) {
      dest[j] = source[i];
    }
    dest[j] = 0;
  }
}

int32_t string_index_of(char* str, char c) {
  if (!str) {
    return -1;
  }
  uint32_t length = strlen(str);
  if (length > 0) {
    for (uint32_t i = 0; i < length; ++i) {
      if (str[i] == c) {
        return i;
      }
    }
  }
  return -1;
}

bool string_to_vec4(char* str, vec4* out_vector) {
  if (!str) {
    return false;
  }
  memset(out_vector, 0, sizeof(vec4));
  int32_t result = sscanf(str, "%f %f %f %f", &out_vector->x, &out_vector->y, &out_vector->z, &out_vector->w);
  return result != -1;
}

bool string_to_vec3(char* str, vec3* out_vector) {
  if (!str) {
    return false;
  }

  memset(out_vector, 0, sizeof(vec3));
  int32_t result = sscanf(str, "%f %f %f", &out_vector->x, &out_vector->y, &out_vector->z);
  return result != -1;
}

bool string_to_vec2(char* str, vec2* out_vector) {
  if (!str) {
    return false;
  }
  memset(out_vector, 0, sizeof(vec2));
  int32_t result = sscanf(str, "%f %f", &out_vector->x, &out_vector->y);
  return result != -1;
}

bool string_to_float(char* str, float* f) {
  if (!str) {
    return false;
  }

  *f = 0;
  int32_t result = sscanf(str, "%f", f);
  return result != -1;
}

bool string_to_double(char* str, double* f) {
  if (!str) {
    return false;
  }

  *f = 0;
  int32_t result = sscanf(str, "%lf", f);
  return result != -1;
}

bool string_to_int8_t(char* str, int8_t* i) {
  if (!str) {
    return false;
  }
  *i = 0;
  int32_t result = sscanf(str, "%hhi", i);
  return result != -1;
}

bool string_to_int16_t(char* str, int16_t* i) {
  if (!str) {
    return false;
  }
  *i = 0;
  int32_t result = sscanf(str, "%hi", i);
  return result != -1;
}

bool string_to_int32_t(char* str, int32_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%i", i);
  return result != -1;
}

bool string_to_int64_t(char* str, int64_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%li", i);
  return result != -1;
}

bool string_to_uint8_t(char* str, uint8_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%hhu", i);
  return result != -1;
}

bool string_to_uint16_t(char* str, uint16_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%hu", i);
  return result != -1;
}

bool string_to_uint32_t(char* str, uint32_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%u", i);
  return result != -1;
}

bool string_to_uint64_t(char* str, uint64_t* i) {
  if (!str) {
    return false;
  }

  *i = 0;
  int32_t result = sscanf(str, "%lu", i);
  return result != -1;
}

bool string_to_bool(char* str, bool* b) {
  if (!str) {
    return false;
  }

  return strcmp(str, "1") == 0 || strcasecmp(str, "true") == 0;
}
