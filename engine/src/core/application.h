#pragma once

#include "defines.h"
#include "platform/platform.h"
#include "logger.h"
#include "core/vmemory.h"
#include "core/event.h"
#include "render/velora_render.h"
#include "core/input.h"

typedef struct _game game;

typedef struct _application_config{
    i16 start_pos_x;
    i16 start_pos_y;
    i16 start_pos_width;
    i16 start_pos_height;
    const char* name;
} application_config;

#include "game_types.h"

typedef struct _application_state{
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
  } application_state;

VAPI b8 application_start(application_state* app_state);

VAPI b8 application_run(application_state* app_state);  