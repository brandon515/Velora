#pragma once

#include "defines.h"
#include "core/platform/platform.h"
#include "render/velora_render.h"
#include "core/input.h"

typedef struct _application_state{
    input_state input;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    render_state *render_state;
    i16 width;
    i16 height;
    f64 last_time;
  } application_state;

 b8 application_start(application_state* app_state);

 b8 application_run(application_state* app_state);  