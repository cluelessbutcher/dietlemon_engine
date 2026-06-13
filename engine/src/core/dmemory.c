#include "dmemory.h"

#include "core/logger.h"

#include "platform/platform.h"

#include <string.h>
#include <stdio.h>

struct memory_stats {
  long long total_allocated;
  long long tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      "
};

static struct memory_stats stats;

void initialize_memory() {
  memset(&stats, 0, sizeof(stats));
}

void shutdown_memory() {}

void* dalloc(long long size, memory_tag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    WARN("dalloc() called with memory tag 'MEMORY_TAG_UNKNOWN' re-assign it");
  }

  stats.total_allocated += size;
  stats.tagged_allocations[tag] += size;

  void* block = palloc(size, false);
  memset(block, 0, size);
  return block;
}

void dfree(void* block, long long size, memory_tag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    WARN("dfree() called with memory tag 'MEMORY_TAG_UNKNOWN' re-assign it");
  }

  stats.total_allocated -= size;
  stats.tagged_allocations[tag] -= size;

  pfree(block, false);
}

char* get_memory_usage_str() {
  const long long gib = 1024 * 1024 * 1024;
  const long long mib = 1024 * 1024;
  const long long kib = 1024;

  char buffer[8000] = "System memory usage (tagged)\n";
  long long offset = strlen(buffer);
  for (int i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
    char unit[4] = "Xib";
    float amount = 1.0f;
    if (stats.tagged_allocations[i] >= gib) {
      unit[0] = 'G';
      amount = stats.tagged_allocations[i] / (float)gib;
    } else if (stats.tagged_allocations[i] >= mib) {
      unit[0] = 'M';
      amount = stats.tagged_allocations[i] / (float)mib;
    } else if (stats.tagged_allocations[i] >= kib) {
      unit[0] = 'K';
      amount = stats.tagged_allocations[i] / (float)kib;
    } else {
      unit[0] = 'B';
      unit[1] = 0;
      amount = (float)stats.tagged_allocations[i];
    }

    int length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
    offset += length;
  }
  char* out_string = strdup(buffer);
  return out_string;
}
