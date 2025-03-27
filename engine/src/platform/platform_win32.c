#include "platform.h"
#include "defines.h"
#include "core/asserts.h"

#if VPLATFORM_WINDOWS

#include "core/logger.h"
#include "core/event.h"
#include "core/vmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <windowsx.h>

typedef struct _internal_state{
  HINSTANCE instance;
  HWND window;
} internal_state;

static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK windows_message_handler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

b8 platform_startup(
platform_state* plat_state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  plat_state->internal_state = platform_allocate(sizeof(internal_state), FALSE);
  internal_state* state = (internal_state*)plat_state->internal_state;

  state->instance = GetModuleHandleA(0);

  const char class_name[] = "Velora Window Class";

  WNDCLASS wc = {
    .style = CS_DBLCLKS,
    .lpfnWndProc = windows_message_handler,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = state->instance,
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

  state->window = CreateWindowExA(
    window_ex_style,
    class_name,
    application_name,
    window_style,
    window_x,window_y,window_width,window_height,
    NULL, // parent window
    NULL, // menu
    state->instance,
    NULL //additional application data
  );

  if(state->window == NULL){
    MessageBoxA(0, "Window was not able to be created", "Error", MB_ICONEXCLAMATION | MB_OK);
    VFATAL("Unable to create the game window");

    return FALSE;
  }

  b8 should_activate = TRUE;
  i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

  ShowWindow(state->window, show_window_command_flags);

  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  clock_frequency = 1.0/(f64)freq.QuadPart;
  QueryPerformanceCounter(&start_time);

  return TRUE;
}

void platform_shutdown(platform_state* plat_state){
  internal_state* state = (internal_state*)plat_state->internal_state;

  if(state->window){
    DestroyWindow(state->window);
    state->window = 0;
  }
}

void* platform_allocate(u64 size, b8 aligned){
  void* ret = malloc(size);
  platform_zero_memory(ret, size);
  return ret;
}

b8 platform_pump_messages(platform_state* plat_state){
  MSG* msg = (MSG*)platform_allocate(sizeof(MSG), FALSE);
  while(PeekMessageA(msg, NULL, 0, 0, PM_REMOVE)){
    TranslateMessage(msg);
    DispatchMessageA(msg);
  }
  platform_free((void*)msg, FALSE);
  return TRUE;
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
  static b8 warned = FALSE;
  if(enable_virtual_terminal(std_handle) == FALSE){
    if(warned == FALSE){ //Only put this message box out once
      MessageBoxA(0, 
        "Virtual terminal isn't avaiable for output, color will be ignored",
        0,
        MB_ICONEXCLAMATION|MB_OK);
        warned = TRUE;
    }
    sprintf(buffer,"%s\n", message);
  }else{
    //Each member corresponds to a log_level
    //FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 text_colors[6] = {30, 31, 33, 32, 34, 36};
    static u8 background_colors[6] = {41, 40, 40, 40, 40, 40};
    sprintf(buffer, "\x1b[%d;%dm%s\x1b[37;40m\n", text_colors[color],background_colors[color], message);
  }
  HANDLE console = GetStdHandle(std_handle);
  OutputDebugStringA(buffer);
  LPDWORD number_written = 0;
  WriteConsole(console, buffer, strlen(buffer), number_written, 0);
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
    case WM_SIZE:{
      //TODO: actually use the updated window size for something
      RECT r;
      GetClientRect(hwnd, &r);
      u32 width = r.right - r.left;
      u32 height = r.bottom - r.top;
      u64 data_size = sizeof(u64)*2;
      event new_event = {
        .event_type = ENGINE_WINDOW_RESIZE,
        .event_data_size = data_size,
        .event_data = vallocate(data_size, MEMORY_TAG_EVENT_DATA),
      };
      u64* dat = (u64*)new_event.event_data;
      dat[0] = width;
      dat[1] = height;
      queue_event(&new_event);
    } break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:{
      // TODO: handle keyboard input
      //b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    } break;
    case WM_MOUSEMOVE:{
      //TODO: handle mouse input
      //i32 x_position = GET_X_LPARAM(lParam);
      //i32 y_position = GET_Y_LPARAM(lParam);
    }break;
    case WM_MOUSEWHEEL:{
      // TODO: handle mouse wheel input
      /*i32 z_delta = GET_WHEEL_DELTA_WPARAM(wParam);
      if(z_delta != 0){
        z_delta = (z_delta < 0) ? -1 : 1;
      }*/
    } break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
      // TODO: handle mouse button input
      //b8 pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
    } break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif //VPLATFORM_WINDOWS