#pragma once

#include "defines.h"

typedef struct hashtable {
  uint64_t element_size;
  uint32_t element_count;
  bool is_pointer_type;
  void* memory;
} hashtable;

DAPI void hashtable_create(uint64_t element_size, uint32_t element_count, void* memory, bool is_pointer_type, hashtable* out_hashtable);
DAPI void hashtable_destroy(hashtable* table);
DAPI bool hashtable_set(hashtable* table, const char* name, void* value);
DAPI bool hashtable_set_ptr(hashtable* table, const char* name, void** value);
DAPI bool hashtable_get(hashtable* table, const char* name, void* out_value);
DAPI bool hashtable_get_ptr(hashtable* table, const char* name, void** out_value);
DAPI bool hashtable_fill(hashtable* table, void* value);
