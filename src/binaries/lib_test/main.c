#include "defines.h"
#include "platform/platform.h"

int main(void){
  platform_state state;
  VEL_CHECK(platform_startup(&state, "Tester", 100, 100, 800, 600));
  platform_shutdown(&state);
  return TRUE;
}