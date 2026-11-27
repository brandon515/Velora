#pragma once
#include "defines.h"

/*
 * The engine's main loop, split out of main() so velora_tests can drive it
 * with fake callbacks and hooks. main() (entry.c) does the platform, renderer
 * and ECS startup and shutdown around it.
 */

// The application's callbacks (entry/entry.h). main() passes velora_init,
// velora_frame and velora_exit.
typedef struct _velora_app_callbacks {
  b8 (*init)(void** state, int argc, char** argv);
  b8 (*frame)(void* state);
  void (*exit)(void* state);
} velora_app_callbacks;

// The engine work done once per frame. main() passes the platform message
// pump and the renderer. Either function may be NULL.
typedef struct _velora_frame_hooks {
  void* context;
  void (*pump_messages)(void* context);
  void (*render)(void* context);
} velora_frame_hooks;

typedef struct _velora_loop_state {
  b8 is_running;
  b8 is_suspended;
  // What the application stored through init's state out-parameter. frame
  // and exit both receive this pointer.
  void* app_data;
} velora_loop_state;

/*
 * Runs the application: init, then frames until the application's frame
 * returns FALSE or an ENGINE_CLOSE_GAME event arrives, then exit.
 * The event system must be running. Returns FALSE if init fails, in which
 * case frame and exit are not called.
 */
b8 velora_run_application(velora_loop_state* loop, const velora_app_callbacks* app, const velora_frame_hooks* hooks, int argc, char** argv);
