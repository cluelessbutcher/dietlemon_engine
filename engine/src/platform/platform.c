#include "platform.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <sys/time.h>

typedef struct internal_state {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t* window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} interanl_state;

bool platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y, int32_t width, int32_t height) {

}

void platform_shutdown(platform_state* plat_state) {

}

bool platform_pump_messages(platform_state* plat_state) {

}

void* platform_allocate(uint64_t size, bool aligned) {

}

void platform_free(void* block, bool aligned) {

}

void platform_console_write(const char* message, uint8_t color) {

}

void platform_console_write_error(const char* message, uint8_t color) {

}

double platform_get_absolute_time() {

}

void platform_sleep(uint64_t ms) {

}