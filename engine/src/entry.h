#pragma once

#ifndef TESTING
#include "core/application.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include "game_types.h"

extern b8 create_game(game* out_game);

int main(void){
  initialize_memory();
  initialize_logging();

  game game_inst;
  if(create_game(&game_inst) == FALSE){
    VFATAL("Could not create instance of the game.");
    return -1;
  }

  if(!game_inst.initialize || !game_inst.on_resize || !game_inst.render || !game_inst.update){
    VFATAL("Game functions not set. Unable to start game instance.")
    return -2;
  }

  if(application_start(&game_inst) == FALSE){
    VFATAL("Application unable to start");
    return -3;
  }

  if(application_run() == FALSE){
    VFATAL("Application could not run main loop");
    return -4;
  }
  shutdown_logging();
  shutdown_memory();
  return 0;
}
#else
int main(void);
#endif