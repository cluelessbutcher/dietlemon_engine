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

    const int32_t msg_length = 32000;
    char out_message[msg_length];
    memset(out_message, 0, sizeof(out_message));

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, 32000, message, arg_ptr);
    va_end(arg_ptr);

    char out_message2[msg_length];
    sprintf(out_message2, "%s%s\n", level_strings[level], out_message);
    
    if (is_error) {
        platform_console_write_error(out_message, level);
    } else {
        platform_console_write(out_message, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, Message: '%s', File: %s, Line: %d\n", expression, message, file, line);
}