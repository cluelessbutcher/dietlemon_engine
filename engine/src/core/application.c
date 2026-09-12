#include "application.h"

#include "game_types.h"
#include "logger.h"

#include "platform/platform.h"

#include "core/dmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"

#include "memory/linear_allocator.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state {
  game* game_inst;
  bool is_running;
  bool is_suspended;
  platform_state platform;
  int16_t width;
  int16_t height;
  clock clock;
  double last_time;
  linear_allocator systems_allocator;
  uint64_t memory_system_memory_requirement;
  void* memory_system_state;
  uint64_t logging_system_memory_requirement;
  void* logging_system_state;
} application_state;

static bool initialized = false;
static application_state* app_state;

bool application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context);
bool application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context);
bool application_on_resized(uint16_t code, void* sender, void* listener_inst, event_context context);

bool application_create(game* game_inst) {
  if (game_inst->application_state) {
    DERROR("application_create called more than once");
    return false;
  }
    
  game_inst->application_state = dallocate(sizeof(application_state), MEMORY_TAG_APPLICATION);
  app_state = game_inst->application_state;
  app_state->game_inst = game_inst;
  app_state->is_running = false;
  app_state->is_suspended = false;
  
  uint64_t systems_allocator_total_size = 64 * 1024 * 1024;
  linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);

  initialize_memory(&app_state->memory_system_memory_requirement, 0);
  app_state->memory_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->memory_system_memory_requirement);
  initialize_memory(&app_state->memory_system_memory_requirement, app_state->memory_system_state);
  
  initialize_logging(&app_state->logging_system_memory_requirement, 0);
  app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
  if (!initialize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
    DERROR("Failed to initialize logging system; shutting down");
    return false;
  }
  
  input_initialize();
  
  if (!event_initialize()) {
    DERROR("Event system failed to initialize, application cannot continue");
    return false;
  }

  event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
  event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

  if (!platform_startup(&app_state->platform,
                        game_inst->app_config.name,
                        game_inst->app_config.start_pos_x,
                        game_inst->app_config.start_pos_y,
                        game_inst->app_config.start_width,
                        game_inst->app_config.start_height)) {
    return false;
  }

  if (!renderer_initialize(game_inst->app_config.name, &app_state->platform)) {
    DFATAL("Failed to initialize renderer, Aborting application");
    return false;
  }
    
  if (!app_state->game_inst->initialize(app_state->game_inst)) {
    DFATAL("Game failed to initialize");
    return false;
  }

  app_state->game_inst->on_resize(app_state->game_inst, app_state->width, app_state->height);

  initialized = true;

  return true;
}

bool application_run() {
  app_state->is_running = true;
  clock_start(&app_state->clock);
  clock_update(&app_state->clock);
  app_state->last_time = app_state->clock.elapsed;
  double running_time = 0;
  uint8_t frame_count = 0;
  double target_frame_seconds = 1.0f/60;

  DINFO(get_memory_usage_str());

  while (app_state->is_running) {
    if (!platform_pump_messages(&app_state->platform)) {
      app_state->is_running = false;
    }

    if (!app_state->is_suspended) {
      clock_update(&app_state->clock);
      double current_time = app_state->clock.elapsed;
      double delta = (current_time - app_state->last_time);
      double frame_start_time = platform_get_absolute_time();
            
      if (!app_state->game_inst->update(app_state->game_inst, (float)delta)) {
        DFATAL("Game update failed, shutting down");
        app_state->is_running = false;
        break;
      }

      if (!app_state->game_inst->render(app_state->game_inst, (float)delta)) {
        DFATAL("Game render failed, shutting down");
        app_state->is_running = false;
        break;
      }

      render_packet packet;
      packet.delta_time = delta;
      renderer_draw_frame(&packet);

      double frame_end_time = platform_get_absolute_time();
      double frame_elapsed_time = frame_end_time - frame_start_time;
      running_time += frame_elapsed_time;
      double remaining_seconds = target_frame_seconds - frame_elapsed_time;

      if (remaining_seconds > 0) {
        uint64_t remaining_ms = (remaining_ms - 1000);
        bool limit_frames = false;
        if (remaining_ms > 0 && limit_frames) {
          platform_sleep(remaining_ms - 1);
        }

        frame_count++;
      }
            
      input_update(delta);
      app_state->last_time = current_time;
    }
  }

  app_state->is_running = false;

  event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    
  event_shutdown();
  input_shutdown();
  renderer_shutdown();
  platform_shutdown(&app_state->platform);
  shutdown_memory();
  
  return true;
}

void application_get_framebuffer_size(uint32_t* width, uint32_t* height) {
  *width = app_state->width;
  *height = app_state->height;
}

bool application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_APPLICATION_QUIT: {
    DINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down\n");
    app_state->is_running = false;
    return true;
  }
  }

  return false;
}

bool application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context) {
  if (code == EVENT_CODE_KEY_PRESSED) {
    uint16_t key_code = context.data.u16[0];
    if (key_code == KEY_ESCAPE) {
      event_context data = {};
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
      return true;
    } else if (key_code == KEY_A) {
      DDEBUG("Explicit - A Pressed");
    } else {
      DDEBUG("'%c' key pressed in window", key_code);
    }
  } else if (code == EVENT_CODE_KEY_RELEASED) {
    uint16_t key_code = context.data.u16[0];
    if (key_code == KEY_B) {
      DDEBUG("Explicit - B released");
    } else {
      DDEBUG("'%c' key released in window", key_code);
    }
  }

  return false;
}

bool application_on_resized(uint16_t code, void* sender, void* listener_inst, event_context context) {
  if (code == EVENT_CODE_RESIZED) {
    uint16_t width = context.data.u16[0];
    uint16_t height = context.data.u16[1];

    if (width != app_state->width || height != app_state->height) {
      app_state->width = width;
      app_state->height = height;

      DDEBUG("Window resize: %i, %i", width, height);

      if (width == 0 || height == 0) {
        DINFO("Window minimized, suspending application");
        app_state->is_suspended = true;
        return true;
      } else {
        if (app_state->is_suspended) {
          DINFO("Window restored, resuming application");
          app_state->is_suspended = false;
        }
        app_state->game_inst->on_resize(app_state->game_inst, width, height);
        renderer_on_resize(width, height);
      }
    }
  }

  return false;
}
