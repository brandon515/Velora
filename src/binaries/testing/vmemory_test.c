#include "vmemory_test.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "core/asserts.h"

#define MEMORY_SIZE 128

void vmemory_test(){
  VERROR("Starting memory test");
  VINFO("Allocating 128 bytes of memory");
  u8* test_mem = (u8*)vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  VINFO("Passed");
  VINFO("Setting all bytes to 1");
  vset_memory((void*)test_mem, 1, MEMORY_SIZE);
  for(int i = 0; i < MEMORY_SIZE; i++){
    VASSERT_MSG(test_mem[i] == 1, "Memory was not able to be set to 1");
  }
  VINFO("Passed");
  VINFO("Copying memory into newly allocated memory");
  u8* copy_mem = (u8*)vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  vcopy_memory((void*)copy_mem, (void*)test_mem, MEMORY_SIZE);
  for(int i = 0; i < MEMORY_SIZE; i++){
    VASSERT_MSG(test_mem[i] == copy_mem[i], "Memory was not able to be copied");
  }
  VINFO("Passed");
  VINFO("Zeroing out memory")
  vzero_memory((void*)test_mem, MEMORY_SIZE);
  for(int i = 0; i < MEMORY_SIZE; i++){
    VASSERT_MSG(test_mem[i] == 0, "Memory was not able to be zeroed out");
  }
  VINFO("Passed");
  VINFO("Freeing memory");
  vfree((void*)copy_mem, MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  vfree((void*)test_mem, MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  VINFO("Passed");
}