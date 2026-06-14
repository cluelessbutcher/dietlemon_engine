#include "event.h"

#include "core/dmemory.h"

#include "containers/darray.h"

#include <string.h>

typedef struct registered_event {
  void* listener;
  PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
  registered_event* events;
} event_code_entry;

#define MAX_MESSAGE_CODES 16384

typedef struct event_system_state {
  event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

static bool is_initialized = false;
static event_system_state state;

bool event_initialize() {
  if (is_initialized) {
    return false;
  }

  is_initialized = false;
  memset(&state, 0, sizeof(state));
  is_initialized = true;

  return true;
}

void event_shutdown() {
  for (short i = 0; i < MAX_MESSAGE_CODES; ++i) {
    if (state.registered[i].events != 0) {
      darray_destroy(state.registered[i].events);
      state.registered[i].events = 0;
    }
  }
}

bool event_register(short code, void* listener, PFN_on_event on_event) {
  if (is_initialized == false) {
    return false;
  }

  if (state.registered[code].events == 0) {
    state.registered[code].events = darray_create(registered_event);
  }

  long long registered_count = darray_length(state.registered[code].events);
  for (long long i = 0; i < registered_count; ++i) {
    if (state.registered[code].events[i].listener == listener) {
      return false;
    }
  }

  registered_event event;
  event.listener = listener;
  event.callback = on_event;
  darray_push(state.registered[code].events, event);

  return true;
}

bool event_unregister(short code, void* listener, PFN_on_event on_event) {
  if (is_initialized == false) {
    return false;
  }

  if (state.registered[code].events == 0) {
    return false;
  }

  long long registered_count = darray_length(state.registered[code].events);
  for (long long i = 0; i < registered_count; ++i) {
    registered_event e = state.registered[code].events[i];
    if (e.listener == listener && e.callback == on_event) {
      registered_event popped_event;
      darray_pop_at(state.registered[code].events, i, &popped_event);
      return true;
    }
  }

  return false;
}

bool event_fire(short code, void* sender, event_context context) {
  if (is_initialized == false) {
    return false;
  }

  if (state.registered[code].events == 0) {
    return false;
  }

  long long registered_count = darray_length(state.registered[code].events);
  for (long long i = 0; i < registered_count; ++i) {
    registered_event e = state.registered[code].events[i];
    if (e.callback(code, sender, e.listener, context)) {
      return true;
    }
  }

  return false;
}
