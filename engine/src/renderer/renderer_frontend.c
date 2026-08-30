#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/dmemory.h"

struct platformm_state;

static renderer_backend* backend = 0;

bool renderer_initialize(const char* application_name, struct platform_state* plat_state) {
    backend = dallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);
    backend->frame_number = 0;

    if (!backend->initialize(backend, application_name, plat_state)) {
        DFATAL("Renderer backend failed to initialize, shutting down");
        return false;
    }

    return true;
}

void renderer_shutdown() {
    backend->shutdown(backend);
    dfree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

bool renderer_begin_frame(float delta_time) {
    return backend->begin_frame(backend, delta_time);
}

bool renderer_end_frame(float delta_time) {
    bool result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

bool renderer_draw_frame(render_packet* packet) {
    if (renderer_begin_frame(packet->delta_time)) {
        bool result = renderer_end_frame(packet->delta_time);
        if (!result) {
            DERROR("renderer_end_frame() failed, application shutting down");
            return false;
        }
    }

    return true;
}
