#include <core/logger.h>
#include <core/asserts.h>
#include <platform/platform.h>

int main(void) {
  DFATAL("This is a test message: %f", 3.14f);
  DERROR("This is a test message: %f", 3.14f);
  DWARN("This is a test message: %f", 3.14f);
  DINFO("This is a test messgae: %f", 3.14f);
  DDEBUG("This is a test message: %f", 3.14f);
  DTRACE("This is a test message: %f", 3.14f);

  platform_state state;
  if (pstartup(&state, "Dietlemon Engine Testbed", 100 ,100, 1280, 720)) {
    while (true) {
      ppumpmsg(&state);
    }
  }

  pshutdown(&state);
  
  return 0;
}
