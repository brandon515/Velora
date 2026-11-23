#include "vmemory_test.h"
#include "test_framework.h"
#include "core/memory/vmemory.h"

#define MEMORY_SIZE 128

static void test_set_copy_zero(void) {
  u8* test_mem = (u8*)vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  REQUIRE(test_mem != NULL, "vallocate returned NULL");

  vset_memory((void*)test_mem, 1, MEMORY_SIZE);
  b8 all_set = TRUE;
  for(int i = 0; i < MEMORY_SIZE; i++){
    all_set = all_set && test_mem[i] == 1;
  }
  CHECK(all_set, "vset_memory did not set every byte to 1");

  u8* copy_mem = (u8*)vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  vcopy_memory((void*)copy_mem, (void*)test_mem, MEMORY_SIZE);
  b8 all_copied = TRUE;
  for(int i = 0; i < MEMORY_SIZE; i++){
    all_copied = all_copied && copy_mem[i] == test_mem[i];
  }
  CHECK(all_copied, "vcopy_memory did not copy every byte");

  vzero_memory((void*)test_mem, MEMORY_SIZE);
  b8 all_zero = TRUE;
  for(int i = 0; i < MEMORY_SIZE; i++){
    all_zero = all_zero && test_mem[i] == 0;
  }
  CHECK(all_zero, "vzero_memory did not zero every byte");

  vfree((void*)copy_mem, MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  vfree((void*)test_mem, MEMORY_SIZE, MEMORY_TAG_APPLICATION);
}

static void test_allocation_tracking(void) {
  u64 before = get_memory_allocated();
  void* block = vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  CHECK(get_memory_allocated() == before + MEMORY_SIZE, "vallocate not counted");
  block = vreallocate(block, MEMORY_SIZE, MEMORY_SIZE * 2, MEMORY_TAG_APPLICATION);
  REQUIRE(block != NULL, "vreallocate returned NULL");
  CHECK(get_memory_allocated() == before + MEMORY_SIZE * 2, "vreallocate not counted");
  vfree(block, MEMORY_SIZE * 2, MEMORY_TAG_APPLICATION);
  CHECK(get_memory_allocated() == before, "vfree not counted");
}

static void test_move_overlapping(void) {
  u8 buf[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vmove_memory(buf + 2, buf, 6);
  u8 right[10] = {0, 1, 0, 1, 2, 3, 4, 5, 8, 9};
  b8 ok = TRUE;
  for(int i = 0; i < 10; i++) {
    ok = ok && buf[i] == right[i];
  }
  CHECK(ok, "vmove_memory shifts right over an overlapping range");

  u8 buf2[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  vmove_memory(buf2, buf2 + 3, 7);
  u8 left[10] = {3, 4, 5, 6, 7, 8, 9, 7, 8, 9};
  ok = TRUE;
  for(int i = 0; i < 10; i++) {
    ok = ok && buf2[i] == left[i];
  }
  CHECK(ok, "vmove_memory shifts left over an overlapping range");
}

static void test_vallocate_zeroes(void) {
  u8* block = vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  REQUIRE(block != NULL, "vallocate returned NULL");
  b8 zero = TRUE;
  for(int i = 0; i < MEMORY_SIZE; i++) {
    zero = zero && block[i] == 0;
  }
  CHECK(zero, "vallocate returns zeroed memory");
  vfree(block, MEMORY_SIZE, MEMORY_TAG_APPLICATION);
}

static void test_reallocate_keeps_contents(void) {
  u8* block = vallocate(MEMORY_SIZE, MEMORY_TAG_APPLICATION);
  for(int i = 0; i < MEMORY_SIZE; i++) {
    block[i] = (u8)i;
  }
  block = vreallocate(block, MEMORY_SIZE, MEMORY_SIZE * 64, MEMORY_TAG_APPLICATION);
  REQUIRE(block != NULL, "vreallocate returned NULL");
  b8 kept = TRUE;
  for(int i = 0; i < MEMORY_SIZE; i++) {
    kept = kept && block[i] == (u8)i;
  }
  CHECK(kept, "vreallocate keeps the old contents");
  vfree(block, MEMORY_SIZE * 64, MEMORY_TAG_APPLICATION);
}

void vmemory_test(void) {
  RUN_TEST(test_set_copy_zero);
  RUN_TEST(test_allocation_tracking);
  RUN_TEST(test_move_overlapping);
  RUN_TEST(test_vallocate_zeroes);
  RUN_TEST(test_reallocate_keeps_contents);
}
