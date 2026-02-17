#ifndef TESTING
#include "core/application.h"
#include "core/logger.h"
#include "core/vmemory.h"
#include "core/event.h"

int main(void){
  initialize_memory();
  initialize_logging();
  initiate_event_system();
  application_state* app_state = vallocate(sizeof(application_state), MEMORY_TAG_APPLICATION);

  if(application_start(app_state) == FALSE){
    VFATAL("Application unable to start");
    return -3;
  }

  if(application_run(app_state) == FALSE){
    VFATAL("Application could not run main loop");
    return -4;
  }
  shutdown_event_system();
  vfree(app_state, sizeof(application_state), MEMORY_TAG_APPLICATION);
  shutdown_logging();
  shutdown_memory();
  return 0;
}
#else
int main(void);
#endif