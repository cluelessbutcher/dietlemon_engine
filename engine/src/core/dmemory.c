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

static struct memory_stats stats;

void initialize_memory() {
    memset(&stats, 0, sizeof(stats));
}

void shutdown_memory() {

}

void* dallocate(uint64_t size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        DWARN("dallocate called with the tag 'MEMORY_TAG_UNKNOWN' please re-class this allocation");
    }

    stats.total_allocated += size;
    stats.tagged_allocations[tag] += size;

    void* block = platform_allocate(size, false);

    memset(block, 0, size);
    return block;
}

void dfree(void* block, uint64_t size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        DWARN("dfree called with the tag 'MEMORY_TAG_UNKNOWN' re-class this allocation");
    }

    stats.total_allocated -= size;
    stats.tagged_allocations[tag] -= size;

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

        int32_t length = snprintf(buffer + offset, 8000, "    %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    char* out_string = strdup(buffer);
    return out_string;
}
