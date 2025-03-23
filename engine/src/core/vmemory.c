#include "vmemory.h"

#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>

typedef struct _mem_stats{
  u64 total_allocated;
  u64 tagged_allocations[MEMORY_TAG_END_TAG];
} memory_stats;

static memory_stats mem_stats;

void initialize_memory(){
  mem_stats.total_allocated = 0;
  for(int i = 0; i < MEMORY_TAG_END_TAG; i++){
    mem_stats.tagged_allocations[i] = 0;
  }
}

void shutdown_memory(){

}

memory_block* vallocate(u64 size, memory_tag tag){
  if(tag == MEMORY_TAG_UNKNOWN){
    VWARN("Memory tag unknown was used, reclassify soon");
  }
  memory_block* ret_block = platform_allocate(sizeof(memory_block), FALSE);
  ret_block->block = platform_allocate(size, FALSE);
  ret_block->size = size;
  ret_block->tag = tag;
  platform_zero_memory(ret_block->block, ret_block->size);

  mem_stats.total_allocated += size;
  mem_stats.tagged_allocations[tag] += size;
  return ret_block;
}

void vfree(memory_block* block){
  platform_free(block->block, FALSE);

  mem_stats.total_allocated -= block->size;
  mem_stats.tagged_allocations[block->tag] -= block->size;

  platform_free(block, FALSE);
}

memory_block* vzero_memory(memory_block* block){
  platform_zero_memory(block->block, block->size);
  return block;
}

memory_block* vcopy_memory(memory_block* dest, memory_block* src){
  u64 true_size = 0;
  if(dest->size > src->size){
    true_size = src->size;
  }else{
    true_size = dest->size;
  }
  platform_copy_memory(dest->block, src->block, true_size);
  return dest;
}

memory_block* vset_memory(memory_block* block, i32 value){
  platform_set_memory(block->block, value, block->size);
  return block;
}

char* get_memory_usage_str(){
  const u64 kib = 1024;
  const u64 mib = kib * 1024;
  const u64 gib = mib * 1024;

  char* buffer = platform_allocate(8192, FALSE);
  sprintf(buffer, "%s", "System memory use (tagged):\n");
  u64 offset = strlen(buffer);
  for(int i = 0; i < MEMORY_TAG_END_TAG; i++){
    char size_desc[4] = "Xib";
    float amount = mem_stats.tagged_allocations[i];
    if(mem_stats.tagged_allocations[i] > gib){
      size_desc[0] = 'g';
      amount /= gib;
    }else if(mem_stats.tagged_allocations[i] > mib){
      size_desc[0] = 'm';
      amount /= mib;
    }else if(mem_stats.tagged_allocations[i] > kib){
      size_desc[0] = 'k';
      amount /= kib;
    }else{
      size_desc[0] = 'b';
      size_desc[1] = 0;
    }
    i32 length = snprintf(buffer+offset, 8192, "\t%s: %.2f %s\n", memory_tag_strings[i], amount, size_desc);
    offset += length;
  }
  return buffer;
}
