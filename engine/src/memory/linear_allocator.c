#include "linear_allocator.h"

#include "core/dmemory.h"
#include "core/logger.h"

#include <string.h>

void linear_allocator_create(uint64_t total_size, void* memory, linear_allocator* out_allocator) {
  if (out_allocator) {
    out_allocator->total_size = total_size;
    out_allocator->allocated = 0;
    out_allocator->owns_memory = memory == 0;
    if (memory) {
      out_allocator->memory = memory;
    } else {
      out_allocator->memory = dallocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
    }
  }
}

void linear_allocator_destroy(linear_allocator* allocator) {
  if (allocator) {
    allocator->allocated = 0;
    if (allocator->owns_memory && allocator->memory) {
      dfree(allocator->memory, allocator->total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
    }
    allocator->memory = 0;
    allocator->total_size = 0;
    allocator->owns_memory = false;
  }
}

void* linear_allocator_allocate(linear_allocator* allocator, uint64_t size) {
  if (allocator && allocator->memory) {
    if (allocator->allocated + size > allocator->total_size) {
      uint64_t remaining = allocator->total_size - allocator->allocated;
      DERROR("linear_allocator_allocate - Tried to allocate %lluB, only %lluB remaining.", size, remaining);
      return 0;
    }
    
    void* block = ((uint8_t*)allocator->memory) + allocator->allocated;
    allocator->allocated += size;
    return block;
  }
  
  DERROR("linear_allocator_allocate - provided allocator not initialized");
  return 0;
}

void linear_allocator_free_all(linear_allocator* allocator) {
  if (allocator && allocator->memory) {
    allocator->allocated = 0;
    memset(allocator->memory, 0, allocator->total_size);
  }
}
