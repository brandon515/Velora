#include <core/logger.h>
#include <core/application.h>
#include <stdlib.h>

int main(void){
  application_config config = {
    .name = "Velora",
    .start_pos_height = 720,
    .start_pos_width = 1280,
    .start_pos_x = 100,
    .start_pos_y = 100
  };
  if(application_start(&config) == FALSE){
    VFATAL("Application unable to start");
    return 1;
  }
  VFATAL("Testeroni");
  VERROR("Testeroni");
  VWARN("Testeroni");
  VINFO("Testeroni");
  VDEBUG("Testeroni");
  VTRACE("Testeroni");
  application_run();
  return 0;
}