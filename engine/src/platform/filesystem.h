#pragma once

#include "defines.h"

typedef struct file_handle {
  void* handle;
  bool is_valid;
} file_handle;

typedef enum file_modes {
  FILE_MODE_READ = 0x1,
  FILE_MODE_WRITE = 0x2
} file_modes;

DAPI bool filesystem_exists(const char* path);
DAPI bool filesystem_open(const char* path, file_modes mode, bool binary, file_handle* out_handle);
DAPI void filesystem_close(file_handle* handle);
DAPI bool filesystem_read_line(file_handle* handle, char** line_buf);
DAPI bool filesystem_read(file_handle* handle, uint64_t data_size, void* out_data, uint64_t* out_bytes_read);
DAPI bool filesystem_read_all_bytes(file_handle* handle, uint8_t** out_bytes, uint64_t* out_bytes_read);
DAPI bool filesystem_write(file_handle* handle, uint64_t data_size, const void* data, uint64_t* out_bytes_written);
