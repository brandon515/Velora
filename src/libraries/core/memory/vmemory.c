#include "core/memory/vmemory.h"

#include "core/logger.h"
#include "core/platform/platform.h"

#include <stdio.h>
#include <string.h>
#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
#endif

typedef struct _mem_stats{
  au64 total_allocated;
  au64 tagged_allocations[MEMORY_TAG_END_TAG];
} memory_stats;

static memory_stats mem_stats;

// The counters are shared by every thread that allocates (vgltf frees from its
// worker threads), so every update and read is atomic. Relaxed ordering is
// enough: they are statistics and order nothing else.
#ifdef __STDC_NO_ATOMICS__
#define STAT_ADD(counter, amount) __atomic_fetch_add(&(counter), (amount), __ATOMIC_RELAXED)
#define STAT_SUB(counter, amount) __atomic_fetch_sub(&(counter), (amount), __ATOMIC_RELAXED)
#define STAT_READ(counter) __atomic_load_n(&(counter), __ATOMIC_RELAXED)
#else
#define STAT_ADD(counter, amount) atomic_fetch_add_explicit(&(counter), (amount), memory_order_relaxed)
#define STAT_SUB(counter, amount) atomic_fetch_sub_explicit(&(counter), (amount), memory_order_relaxed)
#define STAT_READ(counter) atomic_load_explicit(&(counter), memory_order_relaxed)
#endif

void initialize_memory(){
  mem_stats.total_allocated = 0;
  for(int i = 0; i < MEMORY_TAG_END_TAG; i++){
    mem_stats.tagged_allocations[i] = 0;
  }
}

void shutdown_memory(){

}

void* vallocate(u64 size, memory_tag tag){
  if(tag == MEMORY_TAG_UNKNOWN){
    VWARN("Memory tag unknown was used, reclassify soon");
  }
  void* ret_block = platform_allocate(size, FALSE);
  platform_zero_memory(ret_block, size);

  STAT_ADD(mem_stats.total_allocated, size);
  STAT_ADD(mem_stats.tagged_allocations[tag], size);
  return ret_block;
}

void* vreallocate(void *block, u64 old_size, u64 new_size, memory_tag tag){
  void *retPoint = platform_reallocate(block, new_size);
  if(retPoint == NULL){
    return NULL;
  }
  STAT_SUB(mem_stats.total_allocated, old_size);
  STAT_SUB(mem_stats.tagged_allocations[tag], old_size);
  STAT_ADD(mem_stats.total_allocated, new_size);
  STAT_ADD(mem_stats.tagged_allocations[tag], new_size);
  return retPoint;
}

void vfree(void* block, u64 size, memory_tag tag){
  platform_free(block, FALSE);
  STAT_SUB(mem_stats.total_allocated, size);
  STAT_SUB(mem_stats.tagged_allocations[tag], size);
}

void* vzero_memory(void* block, u64 size){
  platform_zero_memory(block, size);
  return block;
}

void* vcopy_memory(void* dest, const void* src, u64 size){
  platform_copy_memory(dest, src, size);
  return dest;
}

void* vset_memory(void* block, i32 value, u64 size){
  platform_set_memory(block, value, size);
  return block;
}

void* vmove_memory(void* dest, const void* src, u64 size){
  return platform_move_memory(dest, src, size);
}

u64 get_memory_allocated() {
  return STAT_READ(mem_stats.total_allocated);
}

u64 get_memory_allocated_by_tag(memory_tag tag) {
  if(tag >= MEMORY_TAG_END_TAG){
    return 0;
  }
  return STAT_READ(mem_stats.tagged_allocations[tag]);
}

char* get_memory_usage_str(){
  const u64 kib = 1024;
  const u64 mib = kib * 1024;
  const u64 gib = mib * 1024;

  char* buffer = platform_allocate(16384, FALSE);
  sprintf(buffer, "%s", "System memory use (tagged):\n");
  u64 offset = strlen(buffer);
  for(int i = 0; i < MEMORY_TAG_END_TAG; i++){
    char size_desc[4] = "Xib";
    u64 bytes = STAT_READ(mem_stats.tagged_allocations[i]);
    float amount = bytes;
    if(bytes > gib){
      size_desc[0] = 'g';
      amount /= gib;
    }else if(bytes > mib){
      size_desc[0] = 'm';
      amount /= mib;
    }else if(bytes > kib){
      size_desc[0] = 'k';
      amount /= kib;
    }else{
      size_desc[0] = 'b';
      size_desc[1] = 0;
    }
    i32 length = snprintf(buffer+offset, 16384, "%s: %.2f %s\n", memory_tag_strings[i], amount, size_desc);
    offset += length;
  }
  return buffer;
}
