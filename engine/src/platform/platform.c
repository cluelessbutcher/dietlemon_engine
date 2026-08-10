#include "platform.h"

#include "core/logger.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct internal_state {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state;

bool platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y, int32_t width, int32_t height) {
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    state->display = XOpenDisplay(NULL);

    XAutoRepeatOff(state->display);

    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection)) {
        DFATAL("Failed to connect to the X server via XCB");
        return false;
    }

    const struct xcb_setup_t* setup = xcb_get_setup(state->connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (int32_t s = screen_p; s > 0; s--) {
        xcb_screen_next(&it);
    }
    state->screen = it.data;

    state->window = xcb_generate_id(state->connection);

    uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    uint32_t value_list[] = {state->screen->black_pixel, event_values};

    xcb_void_cookie_t cookie = xcb_create_window(state->connection,
                                                 XCB_COPY_FROM_PARENT,
                                                 state->window,
                                                 state->screen->root,
                                                 x, y, width, height,
                                                 0,
                                                 XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                 state->screen->root_visual,
                                                 event_mask, value_list);

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(application_name), application_name);

    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state->connection,
                                                                0, strlen("WM_DELETE_WINDOW"),
                                                                "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state->connection,
                                                                   0, strlen("WM_PROTOCOLS"),
                                                                   "WM_PROTOCOLS");

    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(state->connection, wm_protocols_cookie, NULL);
    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

    xcb_map_window(state->connection, state->window);

    uint32_t stream_result = xcb_flush(state->connection);
    if (stream_result <= 0) {
        DFATAL("An error occurred when flushing the stream: %d", stream_result);
        return false;
    }

    return true;
}

void platform_shutdown(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;
    XAutoRepeatOn(state->display);
    xcb_destroy_window(state->connection, state->window);
}

bool platform_pump_messages(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;
    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    bool quit_flagged = false;

    while (event != 0) {
        event = xcb_poll_for_event(state->connection);
        if (event == 0) { break; }
        switch (event->response_type & ~0x80) {
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {

        } break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {

        } break;
        case XCB_MOTION_NOTIFY: {

        } break;
        case XCB_CONFIGURE_NOTIFY: {

        } break;
        case XCB_CLIENT_MESSAGE: {
            cm = (xcb_client_message_event_t*)event;
            if (cm->data.data32[0] == state->wm_delete_win) {
                quit_flagged = true;
            }
        } break;
        default: break;
        }
        free(event);
    }

    return !quit_flagged;
}

void* platform_allocate(uint64_t size, bool alinged) { return malloc(size); }
void platform_free(void* block, bool aligned) { return free(block); }

void platform_console_write(const char* message, uint8_t color) {
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

void platform_console_write_error(const char* message, uint8_t color) {
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

double platform_get_absolute_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platform_sleep(uint64_t ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}
