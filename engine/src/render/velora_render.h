#pragma once

#include "defines.h"

typedef struct _render_state{
  void* internal_render_state;
} render_state;

u8 initiate_render_system(render_state* state, const char* application_name);
void shutdown_render_system(render_state* state);