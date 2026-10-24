#include <core/logger.h>
#include <core/asserts.h>

int main(void) {
  DFATAL("This is a test message: %f", 3.14f);
  DERROR("This is a test message: %f", 3.14f);
  DWARN("This is a test message: %f", 3.14f);
  DINFO("This is a test messgae: %f", 3.14f);
  DDEBUG("This is a test message: %f", 3.14f);
  DTRACE("This is a test message: %f", 3.14f);

  DASSERT(1 == 0);
  
  return 0;
}
