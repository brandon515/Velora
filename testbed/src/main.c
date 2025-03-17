#include <core/asserts.h>
#include <platform/platform.h>

int main(void){
  platform_state *plat_state = malloc(sizeof(platform_state));
  platform_startup(plat_state, "Velora", 100, 100, 1000, 500);
  while(platform_pump_messages(plat_state)){}
  return 0;
}