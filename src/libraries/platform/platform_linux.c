#include "platform/platform.h"
#include "defines.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
typedef struct _linux_state{
  Display *dis;
  Window win;
  XIM xim;
  XIC xic;
  u32 mouseX, mouseY;
} linux_state;

VEXPORT b8 platform_startup(
platform_state* state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
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

  u32 event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask | ResizeRedirectMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

  XSetWindowAttributes windowAttributes = {};
  windowAttributes.colormap = colormap;
  windowAttributes.background_pixel = 0;
  windowAttributes.border_pixel = 0;
  windowAttributes.event_mask = event_mask;

  //int whiteColor = WhitePixel(state->dis, DefaultScreen(state->dis));
  state->win = XCreateWindow(state->dis, RootWindow(state->dis, visInfoTemplate.screen), x, y, width, height, 0,
                              visualInfo->depth, InputOutput, visualInfo->visual,
                              CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);
  
  XSelectInput(state->dis, state->win, event_mask);
  XMapWindow(state->dis, state->win);
  XFlush(state->dis);

  state->xim = XOpenIM(state->dis, 0, 0, 0);
  state->xic = XCreateIC(state->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);
  
  return TRUE;
}

VEXPORT void platform_shutdown(platform_state* state){
  XDestroyWindow(state->dis, state->win);
  XCloseDisplay(state->dis);
}

VEXPORT b8 platform_pump_messages(platform_state* state){
  /*linux_state* lState = (linux_state*)state->internal_state;
  XEvent newXEvent;
  while(XPending(lState->dis)){
    XNextEvent(lState->dis, &newXEvent);
    switch (newXEvent.type)
    {
    case DestroyNotify:
      //create_and_fire_event(ENGINE_CLOSE_GAME, 0, NULL);
      return TRUE;
    case ResizeRequest:{
      XResizeRequestEvent eventDat = newXEvent.xresizerequest;
      resize_data dat = {
        .height = eventDat.height,
        .width = eventDat.width
      };
      create_and_queue_event(ENGINE_WINDOW_RESIZE, sizeof(resize_data), &dat);
    }break;
    case KeyPress:
    case KeyRelease:{
      XKeyEvent eventDat = newXEvent.xkey;
      button_data dat = {
        .button_code = eventDat.keycode,
        .pressed = (newXEvent.type == KeyPress),
      };
      create_and_queue_event(ENGINE_INPUT_BUTTON, sizeof(dat), &dat);
    }break;
    case ButtonPress:
    case ButtonRelease:{
      XButtonEvent eventDat = newXEvent.xbutton;
      // Mouse wheel up
      if(eventDat.button == Button4){
        if(newXEvent.type == ButtonRelease){
          break;
        }
        mouse_wheel_data dat = {
          .direction = 1,
        };
        create_and_queue_event(ENGINE_MOUSE_WHEEL, sizeof(dat), &dat);
        break;
      // Mouse wheel down
      }else if(eventDat.button == Button5){
        if(newXEvent.type == ButtonRelease){
          break;
        }
        mouse_wheel_data dat = {
          .direction = -1,
        };
        create_and_queue_event(ENGINE_MOUSE_WHEEL, sizeof(dat), &dat);
        break;
      }
      mouse_button_data dat = {
        .pressed = (newXEvent.type == ButtonPress),
        .x = lState->mouseX,
        .y = lState->mouseY,
      };
      if(eventDat.button == Button1){
        dat.button_code = MOUSE_L_BUTTON;
      }else if(eventDat.button == Button2){
        dat.button_code = MOUSE_M_BUTTON;
      }else if(eventDat.button == Button3){
        dat.button_code = MOUSE_R_BUTTON;
      }else if(eventDat.button == 8){
        dat.button_code = MOUSE_X1_BUTTON;
      }else if(eventDat.button == 9){
        dat.button_code = MOUSE_X2_BUTTON;
      }else{
        VERROR("Unsupported mouse button pressed");
        VERROR("  Mouse button number: %d", eventDat.button);
        break;
      }
      create_and_queue_event(ENGINE_MOUSE_BUTTON, sizeof(dat), &dat);
    }break;
    case MotionNotify:{
      XPointerMovedEvent eventDat = newXEvent.xmotion;
      lState->mouseX = eventDat.x;
      lState->mouseY = eventDat.y;
      mouse_position_data dat = {
        .x = eventDat.x,
        .y = eventDat.y,
      };
      create_and_queue_event(ENGINE_MOUSE_POSITION, sizeof(dat), &dat);
    }break;
    default:
      break;
    }
  }*/
  return TRUE;
}

VEXPORT void* platform_allocate(u64 size, b8 aligned){
  return malloc(size);
}
VEXPORT void platform_free(void* block, b8 aligned){
  free(block);
}
VEXPORT void* platform_zero_memory(void* block, u64 size){
  return memset(block, 0, size);
}
VEXPORT void* platform_copy_memory(void* dest, const void* source, u64 size){
  return memcpy(dest, source, size);
}
VEXPORT void* platform_set_memory(void* dest, i32 value, u64 size){
  return memset(dest, value, size);
}

VEXPORT void* platform_reallocate(void* block, u64 size){
  return realloc(block, size);
}

void linux_console_write(const char* message, log_level color, FILE* file){
  char buffer[strlen(message)+256];
  static u8 text_colors[6] = {30, 31, 33, 32, 34, 36};
  static u8 background_colors[6] = {41, 40, 40, 40, 40, 40};
  sprintf(buffer, "\x1b[%d;%dm%s\n", text_colors[color],background_colors[color], message);
  fprintf(file, "%s", buffer);
}

VEXPORT void platform_console_write(const char* message, log_level color){
  linux_console_write(message, color, stdout);
}
VEXPORT void platform_console_write_error(const char* message, log_level color){
  linux_console_write(message, color, stderr);
}

VEXPORT f64 platform_get_absolute_time(){
  return 0.0f;
}

VEXPORT void platform_sleep(u64 ms){
  //
}