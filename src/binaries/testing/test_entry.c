#include "darray_test.h"
#include "vmemory_test.h"
#include "core/memory/vmemory.h"
#include "core/logger.h"
int main(void){
  initialize_memory();
  initialize_logging();
  vmemory_test();
  darray_test();
  shutdown_logging();
  shutdown_memory();
  while(TRUE){};
}