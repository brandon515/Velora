#pragma once

#include "defines.h"
#include "core/application.h"
#include "core/vmemory.h"
#include "core/input.h"

typedef struct _game{
  application_config app_config;
 
  b8 (*initialize)(struct _game* game_inst);
  b8 (*update)(struct _game* game_inst, f32 delta_time);
  b8 (*render)(struct _game* game_inst, f32 delta_time);
  void (*on_resize)(struct _game* game_inst, u32 width, u32 height);

  input_state input;
  
  void* state;
} game;