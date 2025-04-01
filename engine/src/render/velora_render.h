#pragma once

#include "defines.h"

typedef struct _render_state{
  void* internal_render_state;
} render_state;

u8 initiate_render_system(render_state* state, const char* application_name);
void shutdown_render_system(render_state* state);

#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>

u8 create_window_surface(render_state* state, HWND window, HINSTANCE handle);
#endif