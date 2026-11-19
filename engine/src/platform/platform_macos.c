#include "platform/platform.h"

#if DPLATFORM_APPLE

#include "core/logger.h"
#include "core/event.h"
#include "core/input.h"
#include "containers/darray.h"

#include "renderer/vulkan/vulkan_platform.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct internal_state {
    GLFWwindow* glfw_window;
} internal_state;

static double start_time = 0;

static void platform_error_callback(int error, const char* description);
static void platform_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void platform_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void platform_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void platform_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void platform_framebuffer_size_callback(GLFWwindow* window, int width, int height);

static keys translate_key(int key);

bool pstartup(platform_state* plat_state,
              const char* application_name,
              int32_t x, int32_t y,
              int32_t width, int32_t height) {
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    glfwSetErrorCallback(platform_error_callback);
    if (!glfwInit()) {
        DFATAL("Failed to initialise GLFW");
        return false;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    state->glfw_window = glfwCreateWindow(width, height, application_name, 0, 0);
    if (!state->glfw_window) {
        DFATAL("Failed to create a window");
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(state->glfw_window, platform_key_callback);
    glfwSetMouseButtonCallback(state->glfw_window, platform_mouse_button_callback);
    glfwSetCursorPosCallback(state->glfw_window, platform_cursor_position_callback);
    glfwSetScrollCallback(state->glfw_window, platform_scroll_callback);
    glfwSetFramebufferSizeCallback(state->glfw_window, platform_framebuffer_size_callback);

    glfwSetWindowPos(state->glfw_window, x, y);
    glfwShowWindow(state->glfw_window);
    start_time = glfwGetTime();

    return true;
}

void pshutdown(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    glfwDestroyWindow(state->glfw_window);
    state->glfw_window = 0;

    glfwSetErrorCallback(0);
    glfwTerminate();
}

bool ppumpmsg(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    glfwPollEvents();
    return !glfwWindowShouldClose(state->glfw_window);
}

void* palloc(uint64_t size, bool aligned) {
    return malloc(size);
}

void pfree(void* block, bool aligned) {
    free(block);
}

void pcw(const char* message, uint8_t colour) {
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

void pcwerr(const char* message, uint8_t colour) {
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    fprintf(stderr, "\033[%sm%s\033[0m", colour_strings[colour], message);
}

double pgat(void) {
    return glfwGetTime();
}

void psleep(uint64_t ms) {
    struct timespec ts = {0};
    ts.tv_sec = (long)(ms / 1000);
    ts.tv_nsec = ((long)ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
}

static void platform_error_callback(int error, const char* description) {
    DERROR("GLFW error (%d): %s", error, description);
}

static void platform_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    bool pressed = action != GLFW_RELEASE;
    keys our_key = translate_key(key);
    if (our_key != KEYS_MAX_KEYS) {
        input_process_key(our_key, pressed);
    }
}

static void platform_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    bool pressed = action != GLFW_RELEASE;
    buttons mouse_button = BUTTON_MAX_BUTTONS;

    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            mouse_button = BUTTON_LEFT;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            mouse_button = BUTTON_MIDDLE;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            mouse_button = BUTTON_RIGHT;
            break;
    }

    if (mouse_button != BUTTON_MAX_BUTTONS) {
        input_process_button(mouse_button, pressed);
    }
}

static void platform_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    input_process_mouse_move((int32_t)xpos, (int32_t)ypos);
}

static void platform_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    input_process_mouse_wheel((int8_t)yoffset);
}

static void platform_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    event_context context;
    context.data.u16[0] = (uint16_t)width;
    context.data.u16[1] = (uint16_t)height;
    event_fire(EVENT_CODE_RESIZED, 0, context);
}

static keys translate_key(int key) {
    switch (key) {
        case GLFW_KEY_SPACE: return KEY_SPACE;
        case GLFW_KEY_COMMA: return KEY_COMMA;
        case GLFW_KEY_MINUS: return KEY_MINUS;
        case GLFW_KEY_PERIOD: return KEY_PERIOD;
        case GLFW_KEY_SLASH: return KEY_SLASH;
        case GLFW_KEY_0: return KEY_NUMPAD0;
        case GLFW_KEY_1: return KEY_NUMPAD1;
        case GLFW_KEY_2: return KEY_NUMPAD2;
        case GLFW_KEY_3: return KEY_NUMPAD3;
        case GLFW_KEY_4: return KEY_NUMPAD4;
        case GLFW_KEY_5: return KEY_NUMPAD5;
        case GLFW_KEY_6: return KEY_NUMPAD6;
        case GLFW_KEY_7: return KEY_NUMPAD7;
        case GLFW_KEY_8: return KEY_NUMPAD8;
        case GLFW_KEY_9: return KEY_NUMPAD9;
        case GLFW_KEY_SEMICOLON: return KEY_SEMICOLON;
        case GLFW_KEY_EQUAL: return KEY_PLUS;
        case GLFW_KEY_A: return KEY_A;
        case GLFW_KEY_B: return KEY_B;
        case GLFW_KEY_C: return KEY_C;
        case GLFW_KEY_D: return KEY_D;
        case GLFW_KEY_E: return KEY_E;
        case GLFW_KEY_F: return KEY_F;
        case GLFW_KEY_G: return KEY_G;
        case GLFW_KEY_H: return KEY_H;
        case GLFW_KEY_I: return KEY_I;
        case GLFW_KEY_J: return KEY_J;
        case GLFW_KEY_K: return KEY_K;
        case GLFW_KEY_L: return KEY_L;
        case GLFW_KEY_M: return KEY_M;
        case GLFW_KEY_N: return KEY_N;
        case GLFW_KEY_O: return KEY_O;
        case GLFW_KEY_P: return KEY_P;
        case GLFW_KEY_Q: return KEY_Q;
        case GLFW_KEY_R: return KEY_R;
        case GLFW_KEY_S: return KEY_S;
        case GLFW_KEY_T: return KEY_T;
        case GLFW_KEY_U: return KEY_U;
        case GLFW_KEY_V: return KEY_V;
        case GLFW_KEY_W: return KEY_W;
        case GLFW_KEY_X: return KEY_X;
        case GLFW_KEY_Y: return KEY_Y;
        case GLFW_KEY_Z: return KEY_Z;
        case GLFW_KEY_GRAVE_ACCENT: return KEY_GRAVE;
        case GLFW_KEY_ESCAPE: return KEY_ESCAPE;
        case GLFW_KEY_ENTER: return KEY_ENTER;
        case GLFW_KEY_TAB: return KEY_TAB;
        case GLFW_KEY_BACKSPACE: return KEY_BACKSPACE;
        case GLFW_KEY_INSERT: return KEY_INSERT;
        case GLFW_KEY_DELETE: return KEY_DELETE;
        case GLFW_KEY_RIGHT: return KEY_RIGHT;
        case GLFW_KEY_LEFT: return KEY_LEFT;
        case GLFW_KEY_DOWN: return KEY_DOWN;
        case GLFW_KEY_UP: return KEY_UP;
        case GLFW_KEY_PAGE_UP: return KEY_PRIOR;
        case GLFW_KEY_PAGE_DOWN: return KEY_NEXT;
        case GLFW_KEY_HOME: return KEY_HOME;
        case GLFW_KEY_END: return KEY_END;
        case GLFW_KEY_CAPS_LOCK: return KEY_CAPITAL;
        case GLFW_KEY_SCROLL_LOCK: return KEY_SCROLL;
        case GLFW_KEY_NUM_LOCK: return KEY_NUMLOCK;
        case GLFW_KEY_PRINT_SCREEN: return KEY_SNAPSHOT;
        case GLFW_KEY_PAUSE: return KEY_PAUSE;
        case GLFW_KEY_F1: return KEY_F1;
        case GLFW_KEY_F2: return KEY_F2;
        case GLFW_KEY_F3: return KEY_F3;
        case GLFW_KEY_F4: return KEY_F4;
        case GLFW_KEY_F5: return KEY_F5;
        case GLFW_KEY_F6: return KEY_F6;
        case GLFW_KEY_F7: return KEY_F7;
        case GLFW_KEY_F8: return KEY_F8;
        case GLFW_KEY_F9: return KEY_F9;
        case GLFW_KEY_F10: return KEY_F10;
        case GLFW_KEY_F11: return KEY_F11;
        case GLFW_KEY_F12: return KEY_F12;
        case GLFW_KEY_F13: return KEY_F13;
        case GLFW_KEY_F14: return KEY_F14;
        case GLFW_KEY_F15: return KEY_F15;
        case GLFW_KEY_F16: return KEY_F16;
        case GLFW_KEY_F17: return KEY_F17;
        case GLFW_KEY_F18: return KEY_F18;
        case GLFW_KEY_F19: return KEY_F19;
        case GLFW_KEY_F20: return KEY_F20;
        case GLFW_KEY_F21: return KEY_F21;
        case GLFW_KEY_F22: return KEY_F22;
        case GLFW_KEY_F23: return KEY_F23;
        case GLFW_KEY_F24: return KEY_F24;
        case GLFW_KEY_KP_0: return KEY_NUMPAD0;
        case GLFW_KEY_KP_1: return KEY_NUMPAD1;
        case GLFW_KEY_KP_2: return KEY_NUMPAD2;
        case GLFW_KEY_KP_3: return KEY_NUMPAD3;
        case GLFW_KEY_KP_4: return KEY_NUMPAD4;
        case GLFW_KEY_KP_5: return KEY_NUMPAD5;
        case GLFW_KEY_KP_6: return KEY_NUMPAD6;
        case GLFW_KEY_KP_7: return KEY_NUMPAD7;
        case GLFW_KEY_KP_8: return KEY_NUMPAD8;
        case GLFW_KEY_KP_9: return KEY_NUMPAD9;
        case GLFW_KEY_KP_DECIMAL: return KEY_DECIMAL;
        case GLFW_KEY_KP_DIVIDE: return KEY_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY: return KEY_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT: return KEY_SUBTRACT;
        case GLFW_KEY_KP_ADD: return KEY_ADD;
        case GLFW_KEY_KP_ENTER: return KEY_ENTER;
        case GLFW_KEY_KP_EQUAL: return KEY_NUMPAD_EQUAL;
        case GLFW_KEY_LEFT_SHIFT: return KEY_LSHIFT;
        case GLFW_KEY_LEFT_CONTROL: return KEY_LCONTROL;
        case GLFW_KEY_LEFT_ALT: return KEY_LMENU;
        case GLFW_KEY_LEFT_SUPER: return KEY_LWIN;
        case GLFW_KEY_RIGHT_SHIFT: return KEY_RSHIFT;
        case GLFW_KEY_RIGHT_CONTROL: return KEY_RCONTROL;
        case GLFW_KEY_RIGHT_ALT: return KEY_RMENU;
        case GLFW_KEY_RIGHT_SUPER: return KEY_RWIN;
        default: return KEYS_MAX_KEYS;
    }
}

void platform_get_required_extension_names(const char*** names_darray) {
    darray_push(*names_darray, &VK_EXT_METAL_SURFACE_EXTENSION_NAME);
}

#endif