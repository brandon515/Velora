#include "application.h"


b8 close_event_handler(event* event, void* state){
  application_state* app_state = (application_state*)state;
  app_state->is_running = FALSE;
  return FALSE;
}

b8 application_start(application_state *app_state){
  app_state->is_running = TRUE;
  app_state->is_suspended = FALSE;
  app_state->width = app_state->game_inst->app_config.start_pos_width;
  app_state->height = app_state->game_inst->app_config.start_pos_height;

  if(platform_startup(
  &app_state->platform, 
  app_state->game_inst->app_config.name,
  app_state->game_inst->app_config.start_pos_x,
  app_state->game_inst->app_config.start_pos_y,
  app_state->game_inst->app_config.start_pos_width,
  app_state->game_inst->app_config.start_pos_height) == FALSE){
    return FALSE;
  }
  
  if(!app_state->game_inst->initialize(app_state->game_inst)){
    VFATAL("Game failed initilization");
    return FALSE;
  }

  initiate_input_system(&app_state->game_inst->input);

  app_state->game_inst->on_resize(
    app_state->game_inst,
    app_state->game_inst->app_config.start_pos_width,
    app_state->game_inst->app_config.start_pos_height
  );

  register_listener(ENGINE_CLOSE_GAME, close_event_handler, app_state);
  register_listener(ENGINE_WINDOW_RESIZE, resize_handler, app_state->platform.render_state);// defined in the renderer code
  return TRUE;
}

b8 application_run(application_state* app_state){
  while(app_state->is_running){
    platform_pump_messages(&app_state->platform);
    render_preframe(app_state->platform.render_state);
    pump_events(99.9f);
    render_frame(app_state->platform.render_state);
    if(app_state->is_suspended == FALSE){
      if(!app_state->game_inst->update(app_state->game_inst, 0.0f)){
        VFATAL("Game update function not able to run");
        app_state->is_running = FALSE;
        break;
      }
      if(!app_state->game_inst->render(app_state->game_inst, 0.0f)){
        VFATAL("Game render function not able to run");
        app_state->is_running = FALSE;
        break;
      }
    }
    render_postframe(app_state->platform.render_state);
  }
  platform_shutdown(&app_state->platform);

  return TRUE;
}