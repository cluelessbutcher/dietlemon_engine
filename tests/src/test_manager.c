#include "test_manager.h"

#include <containers/darray.h>
#include <core/logger.h>
#include <core/clock.h>
#include <core/dstring.h>

typedef struct test_entry {
  PFN_test func;
  char* desc;
} test_entry;

static test_entry* tests;

void test_manager_init() {
  tests = darray_create(test_entry);
}

void test_manager_register_test(uint8_t (*PFN_test)(), char* desc) {
  test_entry e;
  e.func = PFN_test;
  e.desc = desc;
  darray_push(tests, e);
}

void test_manager_run_tests() {
  uint32_t passed = 0;
  uint32_t failed = 0;
  uint32_t skipped = 0;
  
  uint32_t count = darray_length(tests);
  
  clock total_time;
  clock_start(&total_time);
  
  for (uint32_t i = 0; i < count; ++i) {
    clock test_time;
    clock_start(&test_time);
    uint8_t result = tests[i].func();
    clock_update(&test_time);
    
    if (result == true) {
      ++passed;
    } else if (result == BYPASS) {
      ++skipped;
    } else if (result == false) {
      ++failed;
    }
    char status[20];
    string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
    clock_update(&total_time);
    DINFO("Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total)", i + 1, count, skipped, status, test_time.elapsed, total_time.elapsed);
  }
  
  clock_stop(&total_time);
  DINFO("Result: %d passed, %d failed, %d skipped", passed, failed, skipped);
}
