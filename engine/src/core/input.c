#include "core/input.h"
#include "core/event.h"
#include "core/dmemory.h"
#include "core/logger.h"

#include <string.h> 

typedef struct keyboard_state {
    bool keys[256];
} keyboard_state;

typedef struct mouse_state {
    int16_t x;
    int16_t y;
    uint8_t buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

static bool initialized = false;
static input_state state = {};

void input_initialize() {
    memset(&state, 0, sizeof(state));
    initialized = true;
    DINFO("Input subsystemm initialized");
}

void input_shutdown() {
    initialized = false;
}

void input_update(double delta_time) {
    if (!initialized) {
        return;
    }

    memcpy(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    memcpy(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, bool pressed) {
    if (state.keyboard_current.keys[key] != pressed) {
        state.keyboard_current.keys[key] = pressed;
        event_context context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(buttons button, bool pressed) {
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(int16_t x, int16_t y) {
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        state.mouse_current.x = x;
        state.mouse_current.y = y;
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(int8_t z_delta) {
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

bool input_is_key_down(keys key) {
    if (!initialized) {
        return false;
    }

    return state.keyboard_current.keys[key] == true;
}

bool input_is_key_up(keys key) {
    if (!initialized) {
        return true;
    }

    return state.keyboard_current.keys[key] == false;
}

bool input_was_key_down(keys key) {
    if (!initialized) {
        return false;
    }

    return state.keyboard_previous.keys[key] == true;
}

bool input_was_key_up(keys key) {
    if (!initialized) {
        return true;
    }

    return state.keyboard_previous.keys[key] == false;
}

bool input_is_button_down(buttons button) {
    if (!initialized) {
        return false;
    }

    return state.mouse_current.buttons[button] == true;
}

bool input_is_button_up(buttons button) {
    if (!initialized) {
        return false;
    }

    return state.mouse_current.buttons[button] == false;
}

bool input_was_button_down(buttons button) {
    if (!initialized) {
        return true;
    }

    return state.mouse_previous.buttons[button] == true;
}

bool input_was_button_up(buttons button) {
    if (!initialized) {
        return false;
    }

    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(int32_t* x, int32_t* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(int32_t* x, int32_t* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}
