#include "dmemory.h"

#include "core/logger.h"

#include "platform/platform.h"

#include <string.h>
#include <stdio.h>

struct memory_stats {
  uint64_t total_allocated;
  uint64_t tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
  "UNKNOWN          ",
  "ARRAY            ",
  "LINEAR_ALLOCATOR ",
  "DARRAY           ",
  "DICT             ",
  "RING QUEUE       ",
  "BST              ",
  "STRING           ",
  "APPLICATION      ",
  "JOB              ",
  "TEXTURE          ",
  "MATERIAL INSTANCE",
  "RENDERER         ",
  "GAME             ",
  "TRANSFORM        ",
  "ENTITY           ",
  "SCENE            "
};

typedef struct memory_system_state {
  struct memory_stats stats;
  uint64_t alloc_count;
} memory_system_state;

static memory_system_state* state_ptr;

void initialize_memory(uint64_t* memory_requirement, void* state) {
  *memory_requirement = sizeof(memory_system_state);
  if (state == 0) {
    return;
  }
  
  state_ptr = state;
  state_ptr->alloc_count = 0;
  memset(&state_ptr->stats, 0, sizeof(state_ptr->stats));
}

void shutdown_memory() {
  state_ptr = 0;
}

void* dallocate(uint64_t size, memory_tag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    DWARN("dallocate called with the tag 'MEMORY_TAG_UNKNOWN' please re-class this allocation");
  }
  
  if (state_ptr) {
    state_ptr->stats.total_allocated += size;
    state_ptr->stats.tagged_allocations[tag] += size;
    state_ptr->alloc_count++;
  }
  
  void* block = platform_allocate(size, false);

  memset(block, 0, size);
  return block;
}

void dfree(void* block, uint64_t size, memory_tag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    DWARN("dfree called with the tag 'MEMORY_TAG_UNKNOWN' re-class this allocation");
  }

  state_ptr->stats.total_allocated -= size;
  state_ptr->stats.tagged_allocations[tag] -= size;

  platform_free(block, false);
}

char* get_memory_usage_str() {
  const uint64_t gib = 1024 * 1024 * 1024;
  const uint64_t mib = 1024 * 1024;
  const uint64_t kib = 1024;

  char buffer[8000] = "System memory use(tagged)\n";
  uint64_t offset = strlen(buffer);

  for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
    char unit[4] = "Xib";
    float amount = 1.0f;

    if (state_ptr->stats.tagged_allocations[i] >= gib) {
      unit[0] = 'G';
      amount = state_ptr->stats.tagged_allocations[i] / (float)gib;
    } else if (state_ptr->stats.tagged_allocations[i] >= mib) {
      unit[0] = 'M';
      amount = state_ptr->stats.tagged_allocations[i] / (float)mib;
    } else if (state_ptr->stats.tagged_allocations[i] >= kib) {
      unit[0] = 'K';
      amount = state_ptr->stats.tagged_allocations[i] / (float)kib;
    } else {
      unit[0] = 'B';
      unit[1] = 0;
      amount = (float)state_ptr->stats.tagged_allocations[i];
    }

    int32_t length = snprintf(buffer + offset, 8000, "    %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
    offset += length;
  }

  char* out_string = strdup(buffer);
  return out_string;
}

uint64_t get_memory_alloc_count() {
  if (state_ptr) {
    return state_ptr->alloc_count;
  }
  return 0;
} 
