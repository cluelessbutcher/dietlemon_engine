#include "dmemory.h"

#include "core/logger.h"
#include "core/dstring.h"
#include "platform/platform.h"

#include <string.h>
#include <stdio.h>

struct memory_stats {
    uint64_t total_allocated;
    uint64_t tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN       ",
    "ARRAY         ",
    "DARRAY        ",
    "DICT          ",
    "RING_QUEUE    ",
    "BST           ",
    "STRING        ",
    "APPLICATOIN   ",
    "JOB           ",
    "TEXTURE       ",
    "MAT_INST      ",
    "RENDERER      ",
    "GAME          ",
    "TRANSFORM     ",
    "ENTITY        ",
    "ENTITY_MODE   ",
    "SCENE         "
};

static struct memory_stats stats;

void initialize_memory() {
    memset(&stats, 0, sizeof(stats));
}

void shutdown_memory() {

}

void* dalloc(uint64_t size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        DWARN("dalloc() called with the tag: 'MEMORY_TAG_UNKNONW' re-class this allocation");
    }

    stats.total_allocated += size;
    stats.tagged_allocations[tag] += size;
    void* block = palloc(size, false);
    memset(block, 0, size);
    return block;
}

void dfree(void* block, uint64_t size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        DWARN("dfree() calld with the tag: 'MEMORY_TAG_UNKNOWN re-class this de-allocation");
    }
    stats.total_allocated -= size;
    stats.tagged_allocations[tag] -= size;
    pfree(block, false);
}

char* get_memory_usage_str() {
    const uint64_t gib = 1024 * 1024 * 1024;
    const uint64_t mib = 1024 * 1024;
    const uint64_t kib = 1024;
    char buffer[8000] = "System memory use (tagged):\n";
    uint64_t offset = strlen(buffer);
    for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
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

        int32_t length = snprintf(buffer+  offset, 8000, "   %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    char* out_string = dstrdup(buffer);
    return out_string;
}
