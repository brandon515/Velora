#pragma once
#include "core/platform/platform.h"
#include "render/velora_render.h"
#define VELORA_MAIN_CALLBACKS
#ifdef VELORA_MAIN_CALLBACKS
#include "defines.h"

typedef struct _application_state{
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  render_state *render_state;
  i16 width;
  i16 height;
  f64 last_time;
} application_state;

b8 velora_init(void **state, int argc, char **argv);

b8 velora_frame(void *state);

void velora_exit(void *state);
#endif