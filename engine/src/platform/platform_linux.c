#include "platform.h"
#if VPLATFORM_LINUX

#include "core/logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client.h>
#include <errno.h>
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

typedef struct _internal_state{
  struct wl_display* display;
  struct wl_registry* registry;
  struct wl_compositor* compositor;
  struct xdg_wm_base* xdg_base;
  struct wl_seat* seat;

  struct wl_surface* w_surface;
  struct xdg_surface* x_surface;
  struct xdg_toplevel* top_level;

} internal_state;

int allocate_shm_file(size_t size);

static void
xdg_surface_configure(void* data, struct xdg_surface* x_surface, u32 serial){
  xdg_surface_ack_configure(x_surface, serial);
}

static const struct xdg_surface_listener x_surface_listener = {
  .configure = xdg_surface_configure,
};

static void
xdg_ping(void *data, struct xdg_wm_base* xdg_wm_base, u32 serial){
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_base_listener = {
  .ping = xdg_ping,
};

static void
registry_handle_global(void* data, struct wl_registry* registry,
                       u32 name, const char* interface, u32 version){
  internal_state* state = (internal_state*)data;
  if(strcmp(interface, wl_compositor_interface.name) == 0){
    state->compositor = wl_registry_bind(
      registry, 
      name, 
      &wl_compositor_interface,
      4);
  }else if(strcmp(interface, xdg_wm_base_interface.name) == 0){
    state->xdg_base = wl_registry_bind(
      registry,
      name,
      &xdg_wm_base_interface,
      1
    );
    xdg_wm_base_add_listener(
      state->xdg_base,
      &xdg_base_listener,
      state
    );
  }
}

static void
registry_handle_global_remove(void* data, struct wl_registry* registry, u32 name){
  //
}

static const struct wl_registry_listener
registry_listener = {
  .global = registry_handle_global,
  .global_remove = registry_handle_global_remove,
};

b8 platform_startup(
platform_state* plat_state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  plat_state->internal_state = malloc(sizeof(internal_state));
  internal_state* state = (internal_state*)plat_state->internal_state;
  state->display = wl_display_connect(NULL);
  if(!state->display){
    VFATAL("Unable to obtain a wayland display");
    return FALSE;
  }
  state->registry = wl_display_get_registry(state->display);
  wl_registry_add_listener(state->registry, &registry_listener, state);
  wl_display_roundtrip(state->display);

  state->w_surface = wl_compositor_create_surface(state->compositor);
  state->x_surface = xdg_wm_base_get_xdg_surface(
    state->xdg_base,
    state->w_surface
  );
  xdg_surface_add_listener(state->x_surface, &x_surface_listener, state);
  state->top_level = xdg_surface_get_toplevel(state->x_surface);
  xdg_toplevel_set_title(state->top_level, application_name);
  xdg_toplevel_set_app_id(state->top_level, application_name);
  wl_surface_commit(state->w_surface);
  wl_display_roundtrip(state->display);
  wl_surface_commit(state->w_surface);
  return TRUE;
}

void platform_shutdown(platform_state* plat_state){
  internal_state* state = (internal_state*) plat_state->internal_state;
  xdg_toplevel_destroy(state->top_level);
  xdg_surface_destroy(state->x_surface);
  wl_surface_destroy(state->w_surface);
  xdg_wm_base_destroy(state->xdg_base);
  wl_compositor_destroy(state->compositor);
  wl_registry_destroy(state->registry);
  wl_display_disconnect(state->display);
}

b8 platform_pump_messages(platform_state* plat_state){
  internal_state* state = (internal_state*)plat_state->internal_state;
  wl_display_dispatch(state->display);
  return TRUE;
}

void* platform_allocate(u64 size, b8 aligned){
  return malloc(size);
}
void platform_free(void* block, b8 aligned){
  free(block);
}
void* platform_zero_memory(void* block, u64 size){
  return memset(block, 0, size);
}
void* platform_copy_memory(void* dest, const void* source, u64 size){
  return memcpy(dest, source, size);
}
void* platform_set_memory(void* dest, i32 value, u64 size){
  return memset(dest, value, size);
}

void linux_console_write(const char* message, log_level color, FILE* file){
  char buffer[strlen(message)+256];
  static u8 text_colors[6] = {30, 31, 33, 32, 34, 36};
  static u8 background_colors[6] = {41, 40, 40, 40, 40, 40};
  sprintf(buffer, "\x1b[%d;%dm%s\n", text_colors[color],background_colors[color], message);
  fprintf(file, "%s", buffer);
}

void platform_console_write(const char* message, log_level color){
  linux_console_write(message, color, stdout);
}
void platform_console_write_error(const char* message, log_level color){
  linux_console_write(message, color, stderr);
}

f64 platform_get_absolute_time(){
  return 0.0f;
}

void platform_sleep(u64 ms){
  //
}

#endif