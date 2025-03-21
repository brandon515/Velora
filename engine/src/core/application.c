#include "application.h"
#include "platform/platform.h"
#include "game_types.h"
#include "logger.h"

typedef struct _application_state{
  game* game_inst;
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  i16 width;
  i16 height;
  f64 last_time;
} application_state;

static application_state app_state; 
static b8 initialized = FALSE; 

b8 application_start(game* game_inst){
  if(initialized){
    VERROR("Application cannot be dual initialized");
    return FALSE;
  }

  app_state.game_inst = game_inst;

  initialize_logging();
  app_state.is_running = TRUE;
  app_state.is_suspended = FALSE;
  app_state.width = game_inst->app_config.start_pos_width;
  app_state.height = game_inst->app_config.start_pos_height;

  if(platform_startup(
  &app_state.platform, 
  game_inst->app_config.name,
  game_inst->app_config.start_pos_x,
  game_inst->app_config.start_pos_y,
  game_inst->app_config.start_pos_width,
  game_inst->app_config.start_pos_height) == FALSE){
    return FALSE;
  }
  
  if(!app_state.game_inst->initialize(app_state.game_inst)){
    VFATAL("Game failed initilization");
    return FALSE;
  }

  app_state.game_inst->on_resize(
    app_state.game_inst,
    app_state.game_inst->app_config.start_pos_width,
    app_state.game_inst->app_config.start_pos_height
  );

  initialized = TRUE;
  return TRUE;
}

b8 application_run(){
  while(app_state.is_running){
    app_state.is_running = platform_pump_messages(&app_state.platform);
    if(app_state.is_suspended == FALSE){
      if(!app_state.game_inst->update(app_state.game_inst, 0.0f)){
        VFATAL("Game update function not able to run");
        app_state.is_running = FALSE;
        break;
      }
      if(!app_state.game_inst->render(app_state.game_inst, 0.0f)){
        VFATAL("Game render function not able to run");
        app_state.is_running = FALSE;
        break;
      }
    }
  }
  platform_shutdown(&app_state.platform);

  return TRUE;
}