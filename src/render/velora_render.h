#pragma once

#include "defines.h"
#include "core/event.h"
#include "platform/platform.h"

typedef struct _render_state{
  void* internal_render_state;
} render_state;

void shutdown_render_system(render_state* state);

b8 render_preframe(render_state* state);
b8 render_frame(render_state* state);
b8 render_postframe(render_state* state);

b8 resize_handler(event* newEvent, void* state);

b8 initiate_render_system(render_state* state, const char* application_name, platform_state *plat_internal_state);