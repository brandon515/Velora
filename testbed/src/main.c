#include <core/asserts.h>
#include <platform/platform.h>
#include <core/logger.h>
#include <stdlib.h>

int main(void){
  platform_state *plat_state = malloc(sizeof(platform_state));
  platform_startup(plat_state, "Velora", 100, 100, 1280, 720);
  VFATAL("Testeroni");
  VERROR("Testeroni");
  VWARN("Testeroni");
  VINFO("Testeroni");
  VDEBUG("Testeroni");
  VTRACE("Testeroni");
  while(platform_pump_messages(plat_state)){}
  platform_shutdown(plat_state);
  return 0;
}