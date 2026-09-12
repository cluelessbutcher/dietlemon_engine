#include "linear_allocator_tests.h"
#include "../test_manager.h"
#include "../expect.h"

#include <defines.h>

#include <memory/linear_allocator.h>

uint8_t linear_allocator_should_create_and_destroy() {
  linear_allocator alloc;
  linear_allocator_create(sizeof(uint64_t), 0, &alloc);
  
  expect_should_not_be(0, alloc.memory);
  expect_should_be(sizeof(uint64_t), alloc.total_size);
  expect_should_be(0, alloc.allocated);
 
  linear_allocator_destroy(&alloc);
  
  expect_should_be(0, alloc.memory);
  expect_should_be(0, alloc.total_size);
  expect_should_be(0, alloc.allocated);
  
  return true;
}

uint8_t linear_allocator_single_allocation_all_space() {
  linear_allocator alloc;
  linear_allocator_create(sizeof(uint64_t), 0, &alloc);
  
  void* block = linear_allocator_allocate(&alloc, sizeof(uint64_t));
  
  expect_should_not_be(0, block);
  expect_should_be(sizeof(uint64_t), alloc.allocated);
  
  linear_allocator_destroy(&alloc);
  
  return true;
}

uint8_t linear_allocator_multi_allocation_all_space() {
  uint64_t max_allocs = 1024;
  linear_allocator alloc;
  linear_allocator_create(sizeof(uint64_t) * max_allocs, 0, &alloc);
  
  void* block;
  for (uint64_t i = 0; i < max_allocs; ++i) {
    block = linear_allocator_allocate(&alloc, sizeof(uint64_t));
    expect_should_not_be(0, block);
    expect_should_be(sizeof(uint64_t) * (i + 1), alloc.allocated);
  }
  
  linear_allocator_destroy(&alloc);
  
  return true;
}

uint8_t linear_allocator_multi_allocation_over_allocate() {
  uint64_t max_allocs = 3;
  linear_allocator alloc;
  linear_allocator_create(sizeof(uint64_t) * max_allocs, 0, &alloc);
  
  void* block;
  for (uint64_t i = 0; i < max_allocs; ++i) {
    block = linear_allocator_allocate(&alloc, sizeof(uint64_t));
    expect_should_not_be(0, block);
    expect_should_be(sizeof(uint64_t) * (i + 1), alloc.allocated);
  }
  
  DDEBUG("Note: the following error is intentionally caused by this test");
  
  block = linear_allocator_allocate(&alloc, sizeof(uint64_t));
  
  expect_should_be(0, block);
  expect_should_not_be(sizeof(uint64_t) * (max_allocs), alloc.allocated);
  
  linear_allocator_destroy(&alloc);
  
  return true;
}

uint8_t linear_allocator_multi_allocation_all_space_then_free() {
  uint64_t max_allocs = 1024;
  linear_allocator alloc;
  linear_allocator_create(sizeof(uint64_t) * max_allocs, 0, &alloc);
  
  void* block;
  for (uint64_t i = 0; i < max_allocs; ++i) {
    block = linear_allocator_allocate(&alloc, sizeof(uint64_t));
    expect_should_not_be(0, block);
    expect_should_be(sizeof(uint64_t) * (i + 1), alloc.allocated);
  }
  
  linear_allocator_free_all(&alloc);
  expect_should_be(0, alloc.allocated);
  
  linear_allocator_destroy(&alloc);
  
  return true;
}

void linear_allocator_register_tests() {
  test_manager_register_test(linear_allocator_should_create_and_destroy, "Linear allocator should create and destory");
  test_manager_register_test(linear_allocator_single_allocation_all_space, "Linear allocator single allocation all space");
  test_manager_register_test(linear_allocator_multi_allocation_all_space, "Linear allocator multi allocation all space");
  test_manager_register_test(linear_allocator_multi_allocation_over_allocate, "Linear allocator allocation over allocate");
  test_manager_register_test(linear_allocator_multi_allocation_all_space_then_free, "Linear multi allocation all space then free");
}
