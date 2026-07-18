#include <core/logger.h>
#include <core/asserts.h>

#include <platform/platform.h>

int main(void) {
    DFATAL("this is a test message: %f", 3.14f);
    DERROR("this is a test message: %f", 3.14f);
    DWRAN("this is a test message: %f", 3.14f);
    DINFO("this is a test message: %f", 3.14f);
    DDEBUG("this is a test message: %f", 3.14f);
    DTRACE("this is a test message: %f", 3.14f);

    platform_state state;
    if (platform_startup(&state, "Dietlemon Engine", 100, 100, 1280, 720)) {
        while (true) {
            platform_pump_messages(&state);
        }
    }
    platform_shutdown(&state);

    return 0;
}