#include "platform.h"
#if VPLATFORM_LINUX

#include "core/logger.h"
#include "core/event.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

b8 platform_startup(
platform_state* plat_state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  plat_state->internal_state = platform_allocate(sizeof(internal_state), FALSE);
  internal_state* state = (internal_state*) plat_state->internal_state;
  XInitThreads();
  state->dis = XOpenDisplay(NULL);
  if(state->dis == NULL){
    platform_console_write("Unable to connect to XLib display server", LOG_LEVEL_FATAL);
    return FALSE;
  }


  long visualMask = VisualScreenMask;
  int numberofVisuals;
  XVisualInfo visInfoTemplate = {};
  visInfoTemplate.screen = DefaultScreen(state->dis);
  XVisualInfo *visualInfo = XGetVisualInfo(state->dis, visualMask, &visInfoTemplate, &numberofVisuals);
  
  Colormap colormap = XCreateColormap(state->dis, RootWindow(state->dis, visInfoTemplate.screen), visualInfo->visual, AllocNone);

  XSetWindowAttributes windowAttributes = {};
  windowAttributes.colormap = colormap;
  windowAttributes.background_pixel = 0;
  windowAttributes.border_pixel = 0;
  windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

  //int whiteColor = WhitePixel(state->dis, DefaultScreen(state->dis));
  state->win = XCreateWindow(state->dis, RootWindow(state->dis, visInfoTemplate.screen), x, y, width, height, 0,
                              visualInfo->depth, InputOutput, visualInfo->visual,
                              CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);
  
  XSelectInput(state->dis, state->win, ExposureMask | KeyPressMask);
  XMapWindow(state->dis, state->win);
  XFlush(state->dis);
  state->xlib_wm_delete_window = XInternAtom(state->dis, "WM_DELETE_WINDOW", FALSE);
  XSetWMProtocols(state->dis, state->win, &state->xlib_wm_delete_window, 1);

  
  
  return TRUE;
}

void platform_shutdown(platform_state* plat_state){
  internal_state* state = (internal_state*) plat_state->internal_state;
  XDestroyWindow(state->dis, state->win);
  XCloseDisplay(state->dis);
}

b8 platform_pump_messages(platform_state* plat_state){
  internal_state* state = (internal_state*)plat_state->internal_state;
  XEvent newXEvent;
  XCheckWindowEvent(state->dis, state->win, StructureNotifyMask, &newXEvent);
  if(newXEvent.type == MapNotify){
    event new_event ={
      .event_type = ENGINE_CLOSE_GAME,
      .event_data_size = 0,
      .event_data = 0,
    };
    fire_event(&new_event);
  }
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