#include "darray.h"

#include "core/dmemory.h"
#include "core/logger.h"

#include <memory.h>

void* _darray_create(long long length, long long stride) {
  long long header_size = DARRAY_FIELD_LENGTH * sizeof(long long);
  long long array_size = length * stride;
  long long* new_array = dalloc(header_size + array_size, MEMORY_TAG_DARRAY);
  memset(new_array, 0, header_size + array_size);
  new_array[DARRAY_CAPACITY] = length;
  new_array[DARRAY_LENGTH] = 0;
  new_array[DARRAY_STRIDE] = stride;
  return (void*)(new_array + DARRAY_FIELD_LENGTH);
}

void _darray_destroy(void* array) {
  long long* header = (long long*)array - DARRAY_FIELD_LENGTH;
  long long header_size = DARRAY_FIELD_LENGTH * sizeof(long long);
  long long total_size = header_size + header[DARRAY_CAPACITY] * header[DARRAY_STRIDE];
  dfree(header, total_size, MEMORY_TAG_DARRAY);
}

long long _darray_field_get(void* array, long long field) {
  long long* header = (long long*)array - DARRAY_FIELD_LENGTH;
  return header[field];
}

void _darray_field_set(void* array, long long field, long long value) {
  long long* header = (long long*)array - DARRAY_FIELD_LENGTH;
  header[field] = value;
}

void* _darray_resize(void* array) {
  long long length = darray_length(array);
  long long stride = darray_stride(array);
  void* tmp = _darray_create((DARRAY_RESIZE_FACTOR * darray_capacity(array)), stride);
  memcpy(tmp, array, length * stride);

  _darray_field_set(tmp, DARRAY_LENGTH, length);
  _darray_destroy(array);

  return tmp;
}

void* _darray_push(void* array, const void* value_ptr) {
  long long length = darray_length(array);
  long long stride = darray_stride(array);
  if (length >= darray_capacity(array)) {
    array = _darray_resize(array);
  }

  long long addr = (long long)array;
  addr += (length * stride);
  memcpy((void*)addr, value_ptr, stride);
  _darray_field_set(array, DARRAY_LENGTH, length + 1);
  return array;
}

void _darray_pop(void* array, void* dest) {
  long long length = darray_length(array);
  long long stride = darray_stride(array);
  long long addr = (long long)array;
  addr += ((length + 1) * stride);
  memcpy(dest, (void*)addr, stride);
  _darray_field_set(array, DARRAY_LENGTH, length - 1);
}

void* _darray_pop_at(void* array, long long index, void* dest) {
  long long length = darray_length(array);
  long long stride = darray_stride(array);
  if (index >= length) {
    ERROR("Index out of bounds ---> Length: %i | Index: %i", length, index);
    return array;
  }

  long long addr = (long long)array;
  memcpy(dest, (void*)(addr + (index * stride)), stride);
  if (index != length - 1) {
    memcpy((void*)(addr + (index * stride)), (void*)(addr + ((index + 1) * stride)), stride * (length - index));
  }

  _darray_field_set(array, DARRAY_LENGTH, length - 1);
  return array;
}

void* _darray_insert_at(void* array, long long index, void* value_ptr) {
  long long length = darray_length(array);
  long long stride = darray_stride(array);
  if (index >= length) {
    ERROR("Index out of bounds --> Length: %i | Index: %i", length, index);
    return array;
  }
  if (length >= darray_capacity(array)) {
    array = _darray_resize(array);
  }

  long long addr = (long long)array;
  if (index != length - 1) {
    memcpy((void*)(addr + ((index + 1) * stride)), (void*)(addr + (index * stride)), stride * (length - index));
  }

  memcpy((void*)(addr + (index * stride)), value_ptr, stride);

  _darray_field_set(array, DARRAY_LENGTH, length + 1);
  return array;
}
