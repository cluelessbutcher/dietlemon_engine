#pragma once

#include "defines.h"

#include <stdbool.h>

typedef struct event_context {
  union {
    signed long long i64[2];
    unsigned long long u64[2];
    double f64[2];

    signed int i32[4];
    unsigned int u32[4];
    float f32[4];

    signed short i16[8];
    unsigned short u16[8];

    signed char i8[16];
    unsigned char u8[16];

    char c[16];
  } data;
} event_context;

typedef bool (*PFN_on_event)(short code, void* sender, void* listener_inst, event_context data);

bool event_initialize();
void event_shutdown();

DAPI bool event_register(short code, void* listener, PFN_on_event on_event);
DAPI bool event_unregister(short code, void* listener, PFN_on_event on_event);
DAPI bool event_fire(short code, void* sender, event_context context);

typedef enum system_event_code {
    EVENT_CODE_APPLICATION_QUIT = 0x01,
    EVENT_CODE_KEY_PRESSED = 0x02,
    EVENT_CODE_KEY_RELEASED = 0x03,
    EVENT_CODE_BUTTON_PRESSED = 0x04,
    EVENT_CODE_BUTTON_RELEASED = 0x05,
    EVENT_CODE_MOUSE_MOVED = 0x06,
    EVENT_CODE_MOUSE_WHEEL = 0x07,
    EVENT_CODE_RESIZED = 0x08,

    MAX_EVENT_CODE = 0xFF
} system_event_code;
