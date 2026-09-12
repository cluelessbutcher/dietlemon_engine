#include "test_manager.h"

#include "memory/linear_allocator_tests.h"

#include <core/logger.h>

int main(void) {
  test_manager_init();
  
  linear_allocator_register_tests();
  
  DDEBUG("Starting tests...");
  
  test_manager_run_tests();
  
  return 0;
}
