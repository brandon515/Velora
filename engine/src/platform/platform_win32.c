#include "platform.h"
#include "defines.h"
#include "core/asserts.h"

#if VPLATFORM_WINDOWS

#include <Windows.h>
#include <windowsx.h>

typedef struct _internal_state{
  HINSTANCE instance;
  HWND window;
} internal_state;

LRESULT CALLBACK windows_message_handler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  switch (uMsg){
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

b8 platform_startup(
platform_state* plat_state,
const char* application_name,
i32 x,
i32 y,
i32 width,
i32 height){
  plat_state->internal_state = malloc(sizeof(internal_state));
  internal_state* state = (internal_state*)plat_state->internal_state;

  state->instance = GetModuleHandleA(0);

  const char class_name[] = "Velora Window Class";

  WNDCLASS wc = {
    .style = CS_DBLCLKS,
    .lpfnWndProc = windows_message_handler,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = state->instance,
    .hIcon = LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE),
    .hCursor = LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE),
    .lpszClassName = class_name,
  };

  if(RegisterClass(&wc) == 0){
    MessageBoxA(0, "Windows class registration failure", "Error", MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  state->window = CreateWindowEx(
    0,
    class_name,
    application_name,
    WS_OVERLAPPEDWINDOW,
    x,y,width,height,
    NULL, // parent window
    NULL, // menu
    state->instance,
    NULL //additional application data
  );

  if(state->window == NULL){
    MessageBoxA(0, "Window was not able to be created", "Error", MB_ICONEXCLAMATION | MB_OK);
    return FALSE;
  }

  ShowWindow(state->window, SW_SHOWNORMAL);

  return TRUE;
}

b8 platform_pump_messages(platform_state* plat_state){
  MSG msg = {};
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return TRUE;
}

#endif