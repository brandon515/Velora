#include "defines.h"

#if VPLATFORM_WINDOWS
#include "core/platform/platform.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "core/event.h"
#include "core/memory/vmemory.h"
#include "render/velora_render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <windowsx.h>
typedef struct windows_state{
  HINSTANCE instance;
  HWND window;
} windows_state;

static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK windows_message_handler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

b8 platform_startup(
platform_state* state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  state->internal_state = vallocate(sizeof(windows_state), MEMORY_TAG_GAME);
  windows_state* wState = (windows_state*)state->internal_state;
  wState->instance = GetModuleHandleA(0);

  const char class_name[] = "Velora Window Class";

  WNDCLASS wc = {
    .style = CS_DBLCLKS,
    .lpfnWndProc = windows_message_handler,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = wState->instance,
    .hIcon = LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
    .hCursor = LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
    .lpszClassName = class_name,
  };

  if(RegisterClass(&wc) == 0){
    MessageBoxA(0, "Windows class registration failure", "Error", MB_OK | MB_ICONEXCLAMATION);
    VFATAL("Unable to register Window's window class");

    return FALSE;
  }
  u32 window_style = WS_OVERLAPPEDWINDOW;
  u32 window_ex_style = WS_EX_APPWINDOW;

  RECT border_rect = {
    .left = 0,
    .top = 0,
    .right = width,
    .bottom = height
  };
  AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

  u32 window_x = x;
  u32 window_y = y;
  u32 window_width = border_rect.right-border_rect.left;
  u32 window_height = border_rect.bottom-border_rect.top;

  wState->window = CreateWindowExA(
    window_ex_style,
    class_name,
    application_name,
    window_style,
    window_x,window_y,window_width,window_height,
    NULL, // parent window
    NULL, // menu
    wState->instance,
    NULL //additional application data
  );

  if(wState->window == NULL){
    MessageBoxA(0, "Window was not able to be created", "Error", MB_ICONEXCLAMATION | MB_OK);
    VFATAL("Unable to create the game window");

    return FALSE;
  }

  b8 should_activate = TRUE;
  i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

  ShowWindow(wState->window, show_window_command_flags);

  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  clock_frequency = 1.0/(f64)freq.QuadPart;
  QueryPerformanceCounter(&start_time);

  return TRUE;
}

void platform_shutdown(platform_state* state){
  windows_state *wState = (windows_state*)state->internal_state;
  if(wState->window){
    DestroyWindow(wState->window);
    wState->window = 0;
  }
  vfree(state->internal_state, sizeof(windows_state), MEMORY_TAG_GAME);
  state->internal_state = NULL;
}

b8 platform_get_native_window(const platform_state* state, platform_native_window *out_window){
  if(state->internal_state == NULL){
    VERROR("Attempted to get native window before platform startup");
    return FALSE;
  }
  windows_state *wState = (windows_state*)state->internal_state;
  out_window->kind = PLATFORM_WINDOW_WIN32;
  out_window->win32.hinstance = wState->instance;
  out_window->win32.hwnd = wState->window;
  return TRUE;
}

b8 platform_get_window_size(const platform_state* state, u32* out_width, u32* out_height){
  if(state->internal_state == NULL){
    VERROR("Attempted to get window size but platform hasn't been started yet");
    return FALSE;
  }
  windows_state *wState = (windows_state*)state->internal_state;
  RECT winSize;
  GetClientRect(wState->window, &winSize);
  (*out_height) = winSize.bottom;
  (*out_width) = winSize.right;
  return TRUE;
}

void* platform_allocate(u64 size, b8 aligned){
  void* ret = malloc(size);
  return ret;
}

b8 platform_pump_messages(platform_state* plat_state){
  MSG msg = {0};
  while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)){
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
  return TRUE;
}

void* platform_move_memory(void* dest, const void* src, u64 size){
  return memmove(dest, src, size);
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

b8 enable_virtual_terminal(DWORD output){
  HANDLE console = GetStdHandle(output);
  if (console == INVALID_HANDLE_VALUE){
    return FALSE;
  }

  DWORD console_mode = 0;
  if(!GetConsoleMode(console, &console_mode)){
    return FALSE;
  }

  console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if(!SetConsoleMode(console, console_mode)){
    return FALSE;
  }
  return TRUE;
}

void windows_console_write(const char* message, log_level color, DWORD std_handle){
  char* buffer = platform_allocate(strlen(message)+256, FALSE);
  // Fails when the handle isn't a console (e.g. redirected to a pipe or file,
  // as under `meson test` or CI). Write plain text then; never block on a
  // dialog, since there may be no one to dismiss it.
  if(enable_virtual_terminal(std_handle) == FALSE){
    sprintf(buffer,"%s\n", message);
  }else{
    //Each member corresponds to a log_level
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 text_colors[6] = {30, 31, 33, 32, 34, 36};
    static u8 background_colors[6] = {41, 40, 40, 40, 40, 40};
    sprintf(buffer, "\x1b[%d;%dm%s\x1b[37;40m\n", text_colors[color],background_colors[color], message);
  }
  HANDLE output = GetStdHandle(std_handle);
  OutputDebugStringA(buffer);
  // WriteFile works for consoles, pipes and files; WriteConsole only consoles.
  DWORD number_written = 0;
  WriteFile(output, buffer, (DWORD)strlen(buffer), &number_written, NULL);
  platform_free(buffer, FALSE);
}

void platform_console_write(const char* message, log_level color){
  windows_console_write(message, color, STD_OUTPUT_HANDLE);
}
void platform_console_write_error(const char* message, log_level color){
  windows_console_write(message, color, STD_ERROR_HANDLE);
}

f64 platform_get_absolute_time(){
  LARGE_INTEGER now_time;
  QueryPerformanceCounter(&now_time);
  return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms){
  Sleep(ms);
}


LRESULT CALLBACK windows_message_handler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  switch (uMsg){
    case WM_ERASEBKGND:
      return 1;
    case WM_CLOSE:
      {
        event new_event ={
          .event_type = ENGINE_CLOSE_GAME,
          .event_data_size = 0,
          .event_data = 0,
        };
        fire_event(&new_event);
        return 0;
      }
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:{
      b8 pressed = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
      button_data* dat = vallocate(sizeof(button_data), MEMORY_TAG_EVENT_DATA);
      dat->pressed = pressed;
      dat->button_code = wParam;
      event new_event = {
        .event_type = ENGINE_INPUT_BUTTON,
        .event_data_size = sizeof(button_data),
        .event_data = dat,
      };
      queue_event(&new_event);
    } break;
    case WM_MOUSEMOVE:{
      i32 x_position = GET_X_LPARAM(lParam);
      i32 y_position = GET_Y_LPARAM(lParam);
      mouse_position_data* dat = vallocate(sizeof(mouse_position_data), MEMORY_TAG_EVENT_DATA);
      dat->x = x_position;
      dat->y = y_position;
      event new_event = {
        .event_type = ENGINE_MOUSE_POSITION,
        .event_data_size = sizeof(mouse_position_data),
        .event_data = dat,
      };
      queue_event(&new_event);
    }break;
    case WM_MOUSEWHEEL:{
      i32 z_delta = GET_WHEEL_DELTA_WPARAM(wParam);
      if(z_delta != 0){
        z_delta = (z_delta < 0) ? -1 : 1;
      }
      mouse_wheel_data* dat = vallocate(sizeof(mouse_wheel_data), MEMORY_TAG_EVENT_DATA);
      dat->direction = z_delta;
      event new_event = {
        .event_type = ENGINE_MOUSE_WHEEL,
        .event_data_size = sizeof(mouse_wheel_data),
        .event_data = dat,
      };
      queue_event(&new_event);
    } break;
    case WM_SIZE:{
      UINT width = LOWORD(lParam);
      UINT height = HIWORD(lParam);
      resize_data* dat = vallocate(sizeof(resize_data), MEMORY_TAG_EVENT_DATA);
      dat->height = height;
      dat->width = width;
      event new_event = {
        .event_type = ENGINE_WINDOW_RESIZE,
        .event_data_size = sizeof(resize_data),
        .event_data = dat,
      };
      queue_event(&new_event);
    }break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:{
      b8 pressed = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN);
      mouse_button_data* dat = vallocate(sizeof(mouse_button_data), MEMORY_TAG_EVENT_DATA);
      dat->x = GET_X_LPARAM(lParam);
      dat->y = GET_Y_LPARAM(lParam);
      dat->pressed = pressed;
      if(uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP){
        dat->button_code = MOUSE_L_BUTTON;
      }else if(uMsg == WM_RBUTTONUP || uMsg == WM_RBUTTONDOWN){
        dat->button_code = MOUSE_R_BUTTON;
      }else if(uMsg == WM_MBUTTONUP || uMsg == WM_MBUTTONDOWN){
        dat->button_code = MOUSE_M_BUTTON;
      }
      event new_event = {
        .event_type = ENGINE_MOUSE_BUTTON,
        .event_data_size = sizeof(mouse_button_data),
        .event_data = dat,
      };
      queue_event(&new_event);
    } break;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:{
      mouse_button_data* dat = vallocate(sizeof(mouse_button_data), MEMORY_TAG_EVENT_DATA);
      dat->x = GET_X_LPARAM(lParam);
      dat->y = GET_Y_LPARAM(lParam);
      dat->pressed = (uMsg == WM_XBUTTONDOWN);
      if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1){
        dat->button_code = MOUSE_X1_BUTTON;
      }else{
        dat->button_code = MOUSE_X2_BUTTON;
      }
      event new_event = {
        .event_type = ENGINE_MOUSE_BUTTON,
        .event_data_size = sizeof(mouse_button_data),
        .event_data = dat,
      };
      queue_event(&new_event);
      return TRUE; //return true as said in the windows api docs
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

b8 platform_load_library(const char* libURI, platform_library* outLib){
  outLib->internal_lib = (void*)LoadLibraryA(libURI);
  if(!outLib->internal_lib){
    VERROR("Unable to load library '%s'", libURI);
    VERROR("Error: %lu", GetLastError());
    return FALSE;
  }
  u64 size = strlen(libURI)+1;
  outLib->libName = vallocate(size, MEMORY_TAG_APPLICATION);
  vcopy_memory(outLib->libName, libURI, size);
  return TRUE;
}

b8 platform_free_library(platform_library* library){
  b8 retVal = TRUE;
  if(library->internal_lib == NULL){
    VERROR("Freeing null library");
    return FALSE;
  }
  if(!FreeLibrary((HMODULE)library->internal_lib)){
    VERROR("Could not close library '%s'", library->libName);
    VERROR("Error is %lu", GetLastError());
    retVal = FALSE;
  }
  vfree(library->libName, strlen(library->libName)+1, MEMORY_TAG_APPLICATION);
  library->libName = NULL;
  library->internal_lib = NULL;
  return retVal;
}

platform_proc platform_get_function(platform_library* library, const char* funcName){
  FARPROC p = GetProcAddress((HMODULE)library->internal_lib, funcName);
  if(!p){
    VERROR("Unable to import '%s' from '%s' (error %lu)", funcName, library->libName, GetLastError());
  }
  return (platform_proc)p;
}

#endif //VPLATFORM_WINDOWS