#include "application.h"
#include "platform/platform.h"
#include "logger.h"

typedef struct _application_state{
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  i16 width;
  i16 height;
  f64 last_time;
} application_state;

static application_state app_state; 
static b8 initialized = FALSE; 

b8 application_start(application_config* config){
  if(initialized){
    VERROR("Application cannot be dual initialized");
    return FALSE;
  }
  initialize_logging();
  app_state.is_running = TRUE;
  app_state.is_suspended = FALSE;
  app_state.width = config->start_pos_width;
  app_state.height = config->start_pos_height;

  if(platform_startup(
    &app_state.platform, 
    config->name,
    config->start_pos_x,
    config->start_pos_y,
    config->start_pos_width,
    config->start_pos_height) == FALSE){
      return FALSE;
    }
  initialized = TRUE;
  return TRUE;
}

b8 application_run(){
  while(app_state.is_running){
    app_state.is_running = platform_pump_messages(&app_state.platform);
  }
  platform_shutdown(&app_state.platform);

  return TRUE;
}