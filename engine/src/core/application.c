#include "application.h"
#include "game_types.h"

#include "core/logger.h"
#include "core/dmemory.h"
#include "core/event.h"
#include "core/input.h"

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

bool application_on_event(short code, void* sender, void* listener_inst, event_context context);
bool application_on_key(short code, void* sender, void* listener_inst, event_context context);

bool application_create(game* game_inst) {
  if (initialized) {
    ERROR("applicatoin_create() called more than once");
    return false;
  }

  app_state.game_inst = game_inst;
  
  initialize_logging();
  input_initialize();
  
  FATAL("This is a test message");
  ERROR("This is a test message");
  WARN("This is a test message");
  INFO("This is a test message");
  DEBUG("This is a test message");
  TRACE("This is a test message");

  app_state.is_running = true;
  app_state.is_suspended = false;
  
  if (!event_initialize()) {
    ERROR("Failed to initialize event system; shutting down");
    return false;
  }

  event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
  
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

      input_update(0);
    }
  }

  app_state.is_running = false;

  event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
  
  event_shutdown();
  input_shutdown();
  
  pshutdown(&app_state.platform);

  return true;
}

bool application_on_event(short code, void* sender, void* listerner_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_APPLICATION_QUIT: {
    INFO("EVENT_CODE_APPLICATION_QUIT recieved; shutting down");
    return true;
  }
  }

  return false;
}

bool application_on_key(short code, void* sender, void* listener_inst, event_context context) {
  if (code == EVENT_CODE_KEY_PRESSED) {
    short key_code = context.data.u16[0];
    if (key_code == KEY_ESCAPE) {
      event_context data = {};
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
      return true;
    } else if (key_code == KEY_A) {
      DEBUG("'A' Pressed");
    } else {
      DEBUG("'%c' Pressed", key_code);
    }
  } else if (code == EVENT_CODE_KEY_RELEASED) {
    short key_code = context.data.u16[0];
    if (key_code == KEY_B) {
      DEBUG("'B' Released");
    } else {
      DEBUG("'%c' Released");
    }
  }
  return false;
}
