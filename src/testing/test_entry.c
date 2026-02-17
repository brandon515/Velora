#ifdef TESTING
#include "entry.h"

#include <stdlib.h>
#include "testing/darray_test.h"
#include "testing/vmemory_test.h"
#include "core/vmemory.h"
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
#endif