#pragma once

#include "defines.h"

typedef struct _platform_library{
  void *internal_lib;
  char* libName;
}platform_library;

b8 platform_load_library(const char* libURI, platform_library* outLib);
b8 platform_free_library(platform_library* outLib);
void* platform_get_function(const char* funcName, platform_library library);

#if VPLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
typedef struct _platform_state{
  Display *dis;
  Window win;
  XIM xim;
  XIC xic;
  u32 mouseX, mouseY;
} platform_state;
#elif VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
typedef struct platform_state{
  HINSTANCE instance;
  HWND window;
} platform_state;
#endif

typedef enum log_level {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5
} log_level;

b8 platform_startup(
  platform_state* plat_state,
  const char* application_name,
  i32 x,
  i32 y,
  i32 width,
  i32 height
);

void platform_shutdown(platform_state* plat_state);

b8 platform_pump_messages(platform_state* plat_state);

void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_reallocate(void* block, u64 size);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

void platform_console_write(const char* message, log_level color);
void platform_console_write_error(const char* message, log_level color);

f64 platform_get_absolute_time();

void platform_sleep(u64 ms);