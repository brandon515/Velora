#include "core/memory/vbuddy_allocator.h"
#include "core/container/darray.h"
#include "core/logger.h"
#include "core/memory/vmemory.h"
#include "core/utils/vmath.h"
#include "defines.h"

b8 create_buddy_allocator(u64 initialOffset, u64 memorySize, u64 minOrder, vbuddy_allocator *outAlloc){
  vbuddy_allocator scratch = {
    .minOrder = minOrder,
  };
  u64 maxOrder = round_up_to_largest_power_of_two(memorySize)/2; //The memory size cannot be bigger than requested so we round down for this
  maxOrder = log_base_2_unchecked(maxOrder);
  if(maxOrder < minOrder){
    VERROR("Buddy Allocator made with minimum order less than requested maximum order\nMinimum Order: %d\nMaximum Order: %d", minOrder, maxOrder);
    return FALSE;
  }
  scratch.maxOrder = maxOrder;
  for(int i = 0; i < 64; i++){
    scratch.allocated_lists[i] = darray_new(sizeof(vbuddy_array_item));
    scratch.free_lists[i] = darray_new(sizeof(vbuddy_array_item));
  }
  scratch.rootNode.memory.id = U64_MAX;
  scratch.rootNode.memory.offset = initialOffset;
  vbuddy_array_item arrItem = {
    .node = &scratch.rootNode,
  };
  darray_push(scratch.free_lists[maxOrder], &arrItem);
  vcopy_memory(outAlloc, &scratch, sizeof(scratch));
  return TRUE;
}

void free_buddy_allocator(vbuddy_allocator* allocator){
  //
}

vbuddy_node* create_node(vbuddy_allocator* allocator, u64 order){
  if(order+1 > allocator->maxOrder){
    return NULL;
  }
}

void free_node(vbuddy_node* node){
  //
}

u64 buddy_allocate(vbuddy_allocator* allocator, u64 id, u64 size){
  //
}

void buddy_free(vbuddy_allocator* allocator, u64 id){
  //
}
