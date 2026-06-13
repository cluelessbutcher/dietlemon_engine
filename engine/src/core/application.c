#include "application.h"
#include "game_types.h"

#include "core/logger.h"
#include "core/dmemory.h"

#include "platform/platform.h"

typedef struct application_state {
  game* game_inst;
  bool is_running;
  bool is_suspended;
  platform_state platform;
  short width;
  short height;
  double last_time;
} application_state;

static bool initialized = false;
static application_state app_state;

bool application_create(game* game_inst) {
  if (initialized) {
    ERROR("applicatoin_create() called more than once");
    return false;
  }

  app_state.game_inst = game_inst;
  
  initialize_logging();

  FATAL("This is a test message");
  ERROR("This is a test message");
  WARN("This is a test message");
  INFO("This is a test message");
  DEBUG("This is a test message");
  TRACE("This is a test message");

  app_state.is_running = true;
  app_state.is_suspended = false;

  if (!pstartup(&app_state.platform,
		game_inst->app_config.name,
		game_inst->app_config.start_pos_x,
		game_inst->app_config.start_pos_y,
		game_inst->app_config.start_width,
		game_inst->app_config.start_height)) {
    return false;
  }

  if (!app_state.game_inst->initialize(app_state.game_inst)) {
    FATAL("Failed to initialize game");
    return false;
  }

  app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);
  
  initialized = true;
  return true;
}

bool application_run() {
  INFO(get_memory_usage_str());

  while (app_state.is_running) {
    if (!ppump_messages(&app_state.platform)) {
      app_state.is_running = false;
    }

    if (!app_state.is_suspended) {
      if (!app_state.game_inst->update(app_state.game_inst, (float)0)) {
	FATAL("Failed to update game; shutting down");
	app_state.is_running = false;
	break;
      }

      if (!app_state.game_inst->render(app_state.game_inst, (float)0)) {
	FATAL("Failed to render game; shutting down");
	app_state.is_running = false;
	break;
      }
    }
  }

  app_state.is_running = false;
  
  pshutdown(&app_state.platform);

  return true;
}
