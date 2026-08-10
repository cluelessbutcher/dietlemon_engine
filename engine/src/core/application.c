#include "application.h"

#include "game_types.h"
#include "logger.h"

#include "platform/platform.h"

typedef struct application_state {
    game* game_inst;
    bool is_running;
    bool is_suspended;
    platform_state platform;
    int16_t width;
    int16_t height;
    double last_time;
} application_state;

static bool initialized = false;
static application_state app_state;

bool application_create(game* game_inst) {
    if (initialized) {
        DERROR("application_create called more than once");
        return false;
    }

    app_state.game_inst = game_inst;

    initialize_logging();

    DFATAL("a test message: %f", 3.14f);
    DERROR("a test message: %f", 3.14f);
    DWARN("a test message: %f", 3.14f);
    DINFO("a test message: %f", 3.14f);
    DDEBUG("a test message: %f", 3.14f);
    DTRACE("a test message: %f", 3.14f);

    app_state.is_running = true;
    app_state.is_suspended = false;

    if (!platform_startup(&app_state.platform,
                          game_inst->app_config.name,
                          game_inst->app_config.start_pos_x,
                          game_inst->app_config.start_pos_y,
                          game_inst->app_config.start_width,
                          game_inst->app_config.start_height)) {
        return false;
    }

    if (!app_state.game_inst->initialize(app_state.game_inst)) {
        DFATAL("Game failed to initialize");
        return false;
    }

    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);

    initialized = true;

    return true;
}

bool application_run() {
    while (app_state.is_running) {
        if (!platform_pump_messages(&app_state.platform)) {
            app_state.is_running = false;
        }

        if (!app_state.is_suspended) {
            if (!app_state.game_inst->update(app_state.game_inst, (float)0)) {
                DFATAL("Game update failed, shutting down");
                app_state.is_running = false;
                break;
            }

            if (!app_state.game_inst->render(app_state.game_inst, (float)0)) {
                DFATAL("Game render failed, shutting down");
                app_state.is_running = false;
                break;
            }
        }
    }

    app_state.is_running = false;

    platform_shutdown(&app_state.platform);

    return true;
}
