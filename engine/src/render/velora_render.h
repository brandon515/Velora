#pragma once

#include "defines.h"
#include "core/event.h"

typedef struct _render_state{
  void* internal_render_state;
} render_state;

void shutdown_render_system(render_state* state);

u8 render_preframe(render_state* state);
u8 render_frame(render_state* state);
u8 render_postframe(render_state* state);

b8 resize_handler(event* newEvent);

#ifdef VPLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>

u8 initiate_render_system(render_state* state, const char* application_name, HWND window, HINSTANCE handle);
#elif VPLATFORM_LINUX
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "platform/xdg-shell-client-protocol.h"

u8 initiate_render_system(render_state* state, const char* application_name, struct wl_display* display, struct wl_surface* surface);
#endif