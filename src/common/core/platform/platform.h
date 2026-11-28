#pragma once

#include "defines.h"
#include "core/core_api.h"

#define PLATFORM_LOAD_FUNCTION(lib, name, type, dest) \
  ((dest) = (type)platform_get_function((lib), (name)))

typedef struct _platform_library{
  void *internal_lib;
  char* libName;
}platform_library;

typedef void (*platform_proc)(void);

VCORE_API b8 platform_load_library(const char* libURI, platform_library* outLib);
VCORE_API b8 platform_free_library(platform_library* library);
VCORE_API platform_proc platform_get_function(platform_library* library, const char* funcName);

typedef struct _platform_state {
  void *internal_state;
} platform_state;

typedef enum _platform_window_kind {
  PLATFORM_WINDOW_XLIB,
  PLATFORM_WINDOW_WIN32,
} platform_window_kind;

typedef struct _platform_native_window {
  platform_window_kind kind;
  union {
    struct {void* display; u64 window;} xlib;
    struct {void* hinstance; void* hwnd;} win32;
  };
} platform_native_window;

typedef enum log_level {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5
} log_level;

VCORE_API b8 platform_startup(
  platform_state* plat_state,
  const char* application_name,
  i32 x,
  i32 y,
  i32 width,
  i32 height
);

VCORE_API void platform_shutdown(platform_state* plat_state);

VCORE_API b8 platform_pump_messages(platform_state* plat_state);

VCORE_API b8 platform_get_native_window(const platform_state* state, platform_native_window *out_window);
VCORE_API b8 platform_get_window_size(const platform_state* state, u32* out_width, u32* out_height);

VCORE_API void* platform_allocate(u64 size, b8 aligned);
VCORE_API void platform_free(void* block, b8 aligned);
VCORE_API void* platform_reallocate(void* block, u64 size);
VCORE_API void* platform_zero_memory(void* block, u64 size);
VCORE_API void* platform_copy_memory(void* dest, const void* source, u64 size);
VCORE_API void* platform_set_memory(void* dest, i32 value, u64 size);
VCORE_API void* platform_move_memory(void* dest, const void* src, u64 size);

VCORE_API void platform_console_write(const char* message, log_level color);
VCORE_API void platform_console_write_error(const char* message, log_level color);

VCORE_API f64 platform_get_absolute_time();

VCORE_API void platform_sleep(u64 ms);