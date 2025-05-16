#pragma once

#ifndef TESTING
#include "core/application.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include "core/event.h"
#include "game_types.h"

extern b8 create_game(game* out_game);
extern void shutdown_game(game* out_game);

int main(void){
  initialize_memory();
  initialize_logging();
  initiate_event_system();
  application_state* app_state = vallocate(sizeof(application_state), MEMORY_TAG_APPLICATION);
  app_state->game_inst = vallocate(sizeof(game), MEMORY_TAG_GAME);
  if(create_game(app_state->game_inst) == FALSE){
    VFATAL("Could not create instance of the game.");
    return -1;
  }

  if(!app_state->game_inst->initialize || !app_state->game_inst->on_resize || !app_state->game_inst->render || !app_state->game_inst->update){
    VFATAL("Game functions not set. Unable to start game instance.")
    return -2;
  }

  if(application_start(app_state) == FALSE){
    VFATAL("Application unable to start");
    return -3;
  }

  if(application_run(app_state) == FALSE){
    VFATAL("Application could not run main loop");
    return -4;
  }
  shutdown_game(app_state->game_inst);
  vfree(app_state->game_inst, sizeof(game), MEMORY_TAG_GAME);
  shutdown_event_system();
  vfree(app_state, sizeof(application_state), MEMORY_TAG_APPLICATION);
  shutdown_logging();
  shutdown_memory();
  return 0;
}
#else
int main(void);
#endif