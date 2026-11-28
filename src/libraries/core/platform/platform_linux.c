#include "defines.h"

#if VPLATFORM_LINUX
#include "core/memory/vmemory.h"
#include "core/platform/platform.h"
#include "core/event.h"
#include "core/logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <dlfcn.h>

typedef struct _linux_state{
  Display *dis;
  Window win;
  XIM xim;
  XIC xic;
  Atom wm_delete_window;
  u32 width, height;
  u32 mouseX, mouseY;
} linux_state;

b8 platform_startup(
platform_state* state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  state->internal_state = vallocate(sizeof(linux_state), MEMORY_TAG_GAME);
  linux_state* lState = (linux_state*)state->internal_state;
  XInitThreads();
  lState->dis = XOpenDisplay(NULL);
  if(lState->dis == NULL){
    platform_console_write("Unable to connect to XLib display server", LOG_LEVEL_FATAL);
    return FALSE;
  }


  long visualMask = VisualScreenMask;
  int numberofVisuals;
  XVisualInfo visInfoTemplate = {};
  visInfoTemplate.screen = DefaultScreen(lState->dis);
  XVisualInfo *visualInfo = XGetVisualInfo(lState->dis, visualMask, &visInfoTemplate, &numberofVisuals);
  
  Colormap colormap = XCreateColormap(lState->dis, RootWindow(lState->dis, visInfoTemplate.screen), visualInfo->visual, AllocNone);

  // Note: ResizeRedirectMask is intentionally NOT selected. Selecting it makes
  // X11 redirect (suppress) resize requests so the window never actually
  // changes size, which leaves the Vulkan surface capabilities stuck at the
  // initial dimensions. We instead track real geometry via ConfigureNotify
  // (delivered through StructureNotifyMask).
  u32 event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

  XSetWindowAttributes windowAttributes = {};
  windowAttributes.colormap = colormap;
  windowAttributes.background_pixel = 0;
  windowAttributes.border_pixel = 0;
  windowAttributes.event_mask = event_mask;

  //int whiteColor = WhitePixel(lState->dis, DefaultScreen(lState->dis));
  lState->win = XCreateWindow(lState->dis, RootWindow(lState->dis, visInfoTemplate.screen), x, y, width, height, 0,
                              visualInfo->depth, InputOutput, visualInfo->visual,
                              CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);
  
  XSelectInput(lState->dis, lState->win, event_mask);

  lState->width = width;
  lState->height = height;

  // Register for the window manager's close-window protocol so that closing the
  // window sends us a ClientMessage instead of severing the X11 connection.
  lState->wm_delete_window = XInternAtom(lState->dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(lState->dis, lState->win, &lState->wm_delete_window, 1);

  XMapWindow(lState->dis, lState->win);
  XFlush(lState->dis);

  lState->xim = XOpenIM(lState->dis, 0, 0, 0);
  lState->xic = XCreateIC(lState->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);
  
  return TRUE;
}

b8 platform_get_native_window(const platform_state* state, platform_native_window *out_window){
  if(state->internal_state == NULL){
    VERROR("Attempted to get native window before platform startup");
    return FALSE;
  }
  linux_state *lState = (linux_state*)state->internal_state;
  out_window->kind = PLATFORM_WINDOW_XLIB;
  out_window->xlib.window = lState->win;
  out_window->xlib.display = lState->dis;
  return TRUE;
}

b8 platform_get_window_size(const platform_state* state, u32* out_width, u32* out_height){
  if(state->internal_state == NULL){
    VERROR("Attempted to get window size but platform hasn't been started yet");
    return FALSE;
  }
  linux_state *lState = (linux_state*)state->internal_state;
  (*out_height) = lState->height;
  (*out_width) = lState->width;
  return TRUE;
}

void* platform_move_memory(void* dest, const void* src, u64 size){
  return memmove(dest, src, size);
}


void platform_shutdown(platform_state* state){
  linux_state* lState = (linux_state*)state->internal_state;
  XDestroyWindow(lState->dis, lState->win);
  #ifndef VULKAN_RENDER // vulkan actually handles the closing of the display internally
  XCloseDisplay(lState->dis);
  #endif
  vfree(lState, sizeof(linux_state), MEMORY_TAG_GAME);
  state->internal_state = NULL;
}

b8 platform_pump_messages(platform_state* state){
  linux_state* lState = (linux_state*)state->internal_state;
  XEvent newXEvent;
  while(XPending(lState->dis)){
    XNextEvent(lState->dis, &newXEvent);
    switch (newXEvent.type)
    {
    case ClientMessage:
      // The window manager asked us to close (e.g. the user clicked the close
      // button). Queue a graceful shutdown instead of letting X kill us.
      if((Atom)newXEvent.xclient.data.l[0] == lState->wm_delete_window){
        create_and_queue_event(ENGINE_CLOSE_GAME, 0, NULL);
      }
      break;
    case DestroyNotify:
      return TRUE;
    case ConfigureNotify:{
      // ConfigureNotify reports the window's actual geometry. It also fires on
      // moves, so only emit a resize event when the size truly changed.
      XConfigureEvent eventDat = newXEvent.xconfigure;
      if((u32)eventDat.width != lState->width || (u32)eventDat.height != lState->height){
        lState->width = eventDat.width;
        lState->height = eventDat.height;
        resize_data dat = {
          .height = eventDat.height,
          .width = eventDat.width
        };
        create_and_queue_event(ENGINE_WINDOW_RESIZE, sizeof(resize_data), &dat);
      }
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

void* platform_reallocate(void* block, u64 size){
  return realloc(block, size);
}

void linux_console_write(const char* message, log_level color, FILE* file){
  char buffer[strlen(message)+256];
  static u8 text_colors[6] = {30, 31, 33, 32, 34, 36};
  //static u8 background_colors[6] = {41, 40, 40, 40, 40, 40};
  sprintf(buffer, "\x1b[%dm%s\n", text_colors[color], message);
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

b8 platform_load_library(const char* libURI, platform_library* outLib){
  dlerror();
  outLib->internal_lib = dlopen(libURI, RTLD_NOW | RTLD_LOCAL);
  if(outLib->internal_lib == NULL){
    VERROR("Could not open library '%s'", libURI);
    VERROR("Error is %s", dlerror());
    return FALSE;
  }
  u64 size = strlen(libURI)+1;
  outLib->libName = vallocate(size, MEMORY_TAG_APPLICATION);
  vcopy_memory(outLib->libName, libURI, size);
  return TRUE;
}

b8 platform_free_library(platform_library* library){
  b8 retVal = TRUE;
  dlerror();
  if(library->internal_lib == NULL){
    VERROR("Freeing null library");
    return FALSE;
  }
  if(dlclose(library->internal_lib) != 0){
    VERROR("Could not close library '%s'", library->libName);
    VERROR("Error is %s", dlerror());
    retVal = FALSE;
  }
  vfree(library->libName, strlen(library->libName)+1, MEMORY_TAG_APPLICATION);
  library->libName = NULL;
  library->internal_lib = NULL;
  return retVal;
}

platform_proc platform_get_function(platform_library* library, const char* funcName){
  dlerror();
  void* sym = dlsym(library->internal_lib, funcName);
  const char *err = dlerror();
  if (err || !sym){
    VERROR("Unable to import function '%s' from '%s'", funcName, library->libName);
    VERROR("Error is %s", err ? err : "Symbol is NULL");
    return NULL;
  }
  return (platform_proc)sym;
}
#endif