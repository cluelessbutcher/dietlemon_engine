#include "logger.h"
#include "asserts.h"

#include "platform/platform.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

bool initialize_logging() {
  return true;
}

void shutdown_logging() {}

void log_output(log_level level, const char* message, ...) {
  const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
  bool is_error = level < LOG_LEVEL_WARN;

  const int msglen = 32000;
  char out_message[msglen];
  memset(out_message, 0, sizeof(out_message));
  
  __builtin_va_list arg_ptr;
  va_start(arg_ptr, message);
  vsnprintf(out_message, 32000, message, arg_ptr);
  va_end(arg_ptr);

  char out_message2[msglen];
  sprintf(out_message2, "%s%s\n", level_strings[level], out_message);

  if (is_error) {
    pconsole_write_error(out_message2, level);
  } else {
    pconsole_write(out_message2, level);
  }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, int line) {
  log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: %s, file: %s, line: %d\n", expression, message, file, line);
}
