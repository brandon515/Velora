#ifdef TESTING
#include "darray_test.h"

#include "defines.h"
#include "core/asserts.h"
#include "container/darray.h"
#include "core/logger.h"
#include "core/memory/vmemory.h"
void darray_test(){
  VERROR("Starting darray tests");
  darray* test = darray_new(sizeof(u64));
  u64 one = 1;
  u64 two = 2;
  u64 three = 3;
  u64 four = 4;
  VINFO("Adding four numbers to darray");
  test = darray_push(test, &one);
  test = darray_push(test, &two);
  test = darray_push(test, &three);
  test = darray_push(test, &four);
  u64* dat = (u64*)test->data;
  VINFO("Starting testing");
  VASSERT_MSG(dat[0] == 1, "First element isn't one");
  VASSERT_MSG(dat[1] == 2, "Second element isn't two");
  VASSERT_MSG(dat[2] == 3, "Third element isn't three");
  VASSERT_MSG(dat[3] == 4, "Four element isn't four");
  VINFO("Passed");
  VINFO("Popping the third value");
  u64* th = vallocate(sizeof(u64), MEMORY_TAG_APPLICATION);
  darray_pop(test, (void*)th);
  VASSERT_MSG((*th) == 4, "Unable to pop value from dynamic array");
  darray_push(test, (void*)th);
  VINFO("Passed");
  vzero_memory((void*)th, sizeof(u64));
  VINFO("Starting remove test");
  darray_remove(test, 2, (void*)th);
  VASSERT_MSG((*th) == 3, "Value wasn't able to be removed");
  VINFO("Passed");
  VINFO("Starting test to see if removed value can be added in same index");
  VASSERT_MSG(darray_insert(test, (void*)th, 2) != NULL, "Not able to insert value into dynamic array");
  dat = (u64*)test->data;
  VASSERT_MSG(dat[2] == 3, "Wasn't able to insert value correctly");
  VINFO("Passed");
  VINFO("Testing dynamic array delete");
  darray_delete(test, 2);
  VASSERT_MSG(dat[0] == 1, "First element isn't one");
  VASSERT_MSG(dat[1] == 2, "Second element isn't two");
  VASSERT_MSG(dat[2] == 4, "Third element isn't four");
  VINFO("Passed");
}
#endif