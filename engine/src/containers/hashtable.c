#include "hashtable.h"

#include "core/dmemory.h"
#include "core/logger.h"

#include <string.h>

uint64_t hash_name(const char* name, uint32_t element_count) {
  static const uint64_t multiplier = 97;
  unsigned const char* us;
  uint64_t hash = 0;

  for (us = (unsigned const char*)name; *us; us++) {
    hash = hash * multiplier + *us;
  }

  hash %= element_count;
  return hash;
}

void hashtable_create(uint64_t element_size, uint32_t element_count, void* memory, bool is_pointer_type, hashtable* out_hashtable) {
  if (!memory || !out_hashtable) {
    DERROR("hashtable_create() failed pointer to memory and out_hashtable are required");
    return;
  }
  if (!element_count || !element_size) {
    DERROR("element_size and element_count must be a positive non-zero value");
    return;
  }

  out_hashtable->memory = memory;
  out_hashtable->element_count = element_count;
  out_hashtable->element_size = element_size;
  out_hashtable->is_pointer_type = is_pointer_type;
  memset(out_hashtable->memory, 0, element_size * element_count);
}

void hashtable_destroy(hashtable* table) {
  if (table) {
    memset(table, 0, sizeof(hashtable));
  }
}

bool hashtable_set(hashtable* table, const char* name, void* value) {
  if (!table || !name || !value) {
    DERROR("hashtable_set requires table, name and value to exist");
    return false;
  }
  if (table->is_pointer_type) {
    DERROR("hashtable_set should ot be used with tables that have pointer types use hashtable_set_ptr instead");
    return false;
  }

  uint64_t hash = hash_name(name, table->element_count);
  memcpy((uint8_t*)table->memory + (table->element_size * hash), value, table->element_size);
  return true;
}

bool hashtable_set_ptr(hashtable* table, const char* name, void** value) {
  if (!table || !name) {
    DWARN("hashtable_set_ptr requires table and name to exist");
    return false;
  }
  if (!table->is_pointer_type) {
    DERROR("hashtable_set_ptr should not be used with tables that do not have pointer types use hashtable_set instead");
    return false;
  }

  uint64_t hash = hash_name(name, table->element_count);
  ((void**)table->memory)[hash] = value ? *value : 0;
  return true;
}

bool hashtable_get(hashtable* table, const char* name, void* out_value) {
  if (!table || !name || !out_value) {
    DWARN("hashtable_get requires table, name and out_value to exist");
    return false;
  }

  if (table->is_pointer_type) {
    DERROR("hashtable_get should not be used with tables that have pointer types use hashtable_set_ptr instead");
    return false;
  }

  uint64_t hash = hash_name(name, table->element_count);
  memcpy(out_value, (uint8_t*)table->memory + (table->element_size * hash), table->element_size);
  return true;
}

bool hashtable_get_ptr(hashtable* table, const char* name, void** out_value) {
  if (!table || !name || !out_value) {
    DWARN("hashtable_get_ptr requires table, name, and out_value to exist");
    return false;
  }
  if (!table->is_pointer_type) {
    DERROR("hashtable_get_ptr should not be used with tables that do not have pinter types use hashtable_get instead");
    return false;
  }
  uint64_t hash = hash_name(name, table->element_count);
  *out_value = ((void**)table->memory)[hash];
  return *out_value != 0;
}

bool hashtable_fill(hashtable* table, void* value) {
  if (!table || !value) {
    DWARN("hashtable_fill requires table and value to exist");
    return false;
  }
  if (table->is_pointer_type) {
    DERROR("hashtable_fill should not be used with tables that have pointer types");
    return false;
  }
  for (uint32_t i = 0; i < table->element_count; ++i) {
    memcpy((uint8_t*)table->memory + (table->element_size * i), value, table->element_size);
  }
  return true;
}
