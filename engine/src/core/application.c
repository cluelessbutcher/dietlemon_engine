#include "application.h"
#include "game_types.h"
#include "logger.h"
#include "platform/platform.h"
#include "core/dmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"
#include "core/dstring.h"
#include "memory/linear_allocator.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "math/dmath.h"

typedef struct application_state {
  game* game_inst;
  bool is_running;
  bool is_suspended;
  int16_t width;
  int16_t height;
  clock clock;
  double last_time;
  linear_allocator systems_allocator;
  uint64_t event_system_memory_requirement;
  void* event_system_state;
  uint64_t memory_system_memory_requirement;
  void* memory_system_state;
  uint64_t logging_system_memory_requirement;
  void* logging_system_state;
  uint64_t input_system_memory_requirement;
  void* input_system_state;
  uint64_t platform_system_memory_requirement;
  void* platform_system_state;
  uint64_t renderer_system_memory_requirement;
  void* renderer_system_state;
  
  uint64_t texture_system_memory_requirement;
  void* texture_system_state;
  uint64_t material_system_memory_requirement;
  void* material_system_state;
  uint64_t geometry_system_memory_requirement;
  void* geometry_system_state;
  geometry* test_geometry;
} application_state;

static application_state* app_state;

bool application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context);
bool application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context);
bool application_on_resized(uint16_t code, void* sender, void* listener_inst, event_context context);

bool event_on_debug_event(uint16_t code, void* sender, void* listener_inst, event_context data) {
  const char* names[3] = {
    "cobblestone",
    "paving",
    "paving2",
  };
  static int8_t choice = 2;
  const char* old_name = names[choice];
  choice++;
  choice %= 3;
  if (app_state->test_geometry) {
    app_state->test_geometry->material->diffuse_map.texture = texture_system_acquire(names[choice], true);
    if (!app_state->test_geometry->material->diffuse_map.texture) {
      DWARN("event_on_debug_event() no texture using default");
      app_state->test_geometry->material->diffuse_map.texture = texture_system_get_default_texture();
    }
    texture_system_release(old_name);
  }
  return true;
}

bool application_create(game* game_inst) {
    if (game_inst->application_state) {
        DERROR("application_create called more than once.");
        return false;
    }
    game_inst->application_state = dallocate(sizeof(application_state), MEMORY_TAG_APPLICATION);
    app_state = game_inst->application_state;
    app_state->game_inst = game_inst;
    app_state->is_running = false;
    app_state->is_suspended = false;
    uint64_t systems_allocator_total_size = 64 * 1024 * 1024;
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);
    event_system_initialize(&app_state->event_system_memory_requirement, 0);
    app_state->event_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
    event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);
    memory_system_initialize(&app_state->memory_system_memory_requirement, 0);
    app_state->memory_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->memory_system_memory_requirement);
    memory_system_initialize(&app_state->memory_system_memory_requirement, app_state->memory_system_state);
    initialize_logging(&app_state->logging_system_memory_requirement, 0);
    app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if (!initialize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
        DERROR("Failed to initialize logging system; shutting down.");
        return false;
    }
    input_system_initialize(&app_state->input_system_memory_requirement, 0);
    app_state->input_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
    input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);
    event_register(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    
    platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    app_state->platform_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
    if (!platform_system_startup(
            &app_state->platform_system_memory_requirement,
            app_state->platform_system_state,
            game_inst->app_config.name,
            game_inst->app_config.start_pos_x,
            game_inst->app_config.start_pos_y,
            game_inst->app_config.start_width,
            game_inst->app_config.start_height)) {
        return false;
    }
    renderer_system_initialize(&app_state->renderer_system_memory_requirement, 0, 0);
    app_state->renderer_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if (!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, game_inst->app_config.name)) {
        DFATAL("Failed to initialize renderer. Aborting application.");
        return false;
    }
    texture_system_config texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_initialize(&app_state->texture_system_memory_requirement, 0, texture_sys_config);
    app_state->texture_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->texture_system_memory_requirement);
    if (!texture_system_initialize(&app_state->texture_system_memory_requirement, app_state->texture_system_state, texture_sys_config)) {
      DFATAL("Failed to initialize texture system so the applicatoin cannot continue");
      return false;
    }
    material_system_config material_sys_config;
    material_sys_config.max_material_count = 4096;
    material_system_initialize(&app_state->material_system_memory_requirement, 0, material_sys_config);
    app_state->material_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->material_system_memory_requirement);
    if (!material_system_initialize(&app_state->material_system_memory_requirement, app_state->material_system_state, material_sys_config)) {
      DFATAL("Failed to initialize material system; appilcatoin cannot continue");
      return false;
    }

    geometry_system_config geometry_sys_config;
    geometry_sys_config.max_geometry_count = 4096;
    geometry_system_initialize(&app_state->geometry_system_memory_requirement, 0, geometry_sys_config);
    app_state->geometry_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->geometry_system_memory_requirement);
    if (!geometry_system_initialize(&app_state->geometry_system_memory_requirement, app_state->geometry_system_state, geometry_sys_config)) {
      DFATAL("Failed to initialize geometry system. Application cannot continue.");
      return false;
    }

    geometry_config g_config = geometry_system_generate_plane_config(10.0f, 5.0f, 5, 5, 5.0f, 2.0f, "test geometry", "test_material");
    app_state->test_geometry = geometry_system_acquire_from_config(g_config, true);
    dfree(g_config.vertices, sizeof(vertex_3d) * g_config.vertex_count, MEMORY_TAG_ARRAY);
    dfree(g_config.indices, sizeof(uint32_t) * g_config.index_count, MEMORY_TAG_ARRAY);
    
    if (!app_state->game_inst->initialize(app_state->game_inst)) {
      DFATAL("Game failed to initialize.");
      return false;
    }
    app_state->game_inst->on_resize(app_state->game_inst, app_state->width, app_state->height);
    return true;
}

bool application_run() {
    app_state->is_running = true;
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed;
    double running_time = 0;
    uint8_t frame_count = 0;
    double target_frame_seconds = 1.0f / 60;
    DINFO(get_memory_usage_str());
    while (app_state->is_running) {
        if (!platform_pump_messages()) {
            app_state->is_running = false;
        }
        if (!app_state->is_suspended) {
            clock_update(&app_state->clock);
            double current_time = app_state->clock.elapsed;
            double delta = (current_time - app_state->last_time);
            double frame_start_time = platform_get_absolute_time();
            if (!app_state->game_inst->update(app_state->game_inst, (float)delta)) {
                DFATAL("Game update failed, shutting down.");
                app_state->is_running = false;
                break;
            }
            if (!app_state->game_inst->render(app_state->game_inst, (float)delta)) {
                DFATAL("Game render failed, shutting down.");
                app_state->is_running = false;
                break;
            }
            render_packet packet;
            packet.delta_time = delta;
	    geometry_render_data test_render;
	    test_render.geometry = app_state->test_geometry;
	    test_render.model = mat4_identity();
	    packet.geometry_count = 1;
	    packet.geometries = &test_render;
	    
	    renderer_draw_frame(&packet);
            double frame_end_time = platform_get_absolute_time();
            double frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            double remaining_seconds = target_frame_seconds - frame_elapsed_time;
            if (remaining_seconds > 0) {
                uint64_t remaining_ms = (remaining_seconds * 1000);
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
    event_unregister(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    
    input_system_shutdown(app_state->input_system_state);
    geometry_system_shutdown(app_state->geometry_system_state);
    material_system_shutdown(app_state->material_system_state);
    texture_system_shutdown(app_state->texture_system_state);
    renderer_system_shutdown(app_state->renderer_system_state);
    platform_system_shutdown(app_state->platform_system_state);
    memory_system_shutdown(app_state->memory_system_state);
    event_system_shutdown(app_state->event_system_state);
    return true;
}

void application_get_framebuffer_size(uint32_t* width, uint32_t* height) {
    *width = app_state->width;
    *height = app_state->height;
}

bool application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            DINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.\n");
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
            DDEBUG("Explicit - A key pressed!");
        } else {
            DDEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        uint16_t key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            DDEBUG("Explicit - B key released!");
        } else {
            DDEBUG("'%c' key released in window.", key_code);
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
                DINFO("Window minimized, suspending application.");
                app_state->is_suspended = true;
                return true;
            } else {
                if (app_state->is_suspended) {
                    DINFO("Window restored, resuming application.");
                    app_state->is_suspended = false;
                }
                app_state->game_inst->on_resize(app_state->game_inst, width, height);
                renderer_on_resized(width, height);
            }
        }
    }
    return false;
}
