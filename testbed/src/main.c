#include <core/asserts.h>

int main(void){
  VASSERT(TRUE);
  VASSERT_MSG(FALSE,"See, done");
  return 0;
}