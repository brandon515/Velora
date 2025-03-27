#include "darray.h"

#include "core/vmemory.h"
#include "core/logger.h"

darray* darray_new(u64 stride){
  darray* ret_arr = vallocate(sizeof(darray), MEMORY_TAG_DARRAY);
  u64 size_in_bytes = STARTING_LENGTH*stride;
  ret_arr->data = vallocate(size_in_bytes, MEMORY_TAG_DARRAY);
  vzero_memory(ret_arr->data, size_in_bytes);
  ret_arr->cap = STARTING_LENGTH;
  ret_arr->length = 0;
  ret_arr->stride = stride;
  return ret_arr;
}

darray* darray_reallocate(darray* arr){
  u8* buffer[arr->cap*arr->stride];
  u64 new_cap = arr->cap*REALLOCATE_MULTIPLIER;
  vcopy_memory(&buffer, arr->data, arr->cap*arr->stride);
  vfree(arr->data, arr->cap*arr->stride, MEMORY_TAG_DARRAY);
  arr->data = vallocate(new_cap, MEMORY_TAG_DARRAY);
  vzero_memory(arr->data, new_cap);
  vcopy_memory(arr->data, &buffer, arr->cap*arr->stride);
  arr->cap = new_cap;
  return arr;
}

darray* darray_push(darray* arr, void* data){
  darray* ret_arr = arr;
  if(ret_arr->length+1 >= ret_arr->cap){
    ret_arr = darray_reallocate(arr);
  }
  vcopy_memory(ret_arr->data+(ret_arr->length*ret_arr->stride), data, ret_arr->stride);
  ret_arr->length++;
  return ret_arr;
}

darray* darray_pop(darray* arr, void* dest){
  if(arr->length == 0){
    VWARN("Attempted to pop an item off an empty dynamic array");
    return NULL;
  }
  arr->length--;
  u64 mem_location = arr->length*arr->stride;
  vcopy_memory(dest, arr->data+(mem_location), arr->stride);
  return arr;
}

darray* darray_insert(darray* arr, void* data, u64 index){
  darray* ret_arr = arr;
  if(index >= arr->length){
    VWARN("Attempted to insert into a dynamic array past it's length");
    return NULL;
  }else if(arr->length+1 >= arr->cap){
    ret_arr = darray_reallocate(arr);
  }
  u8 buffer[ret_arr->cap*ret_arr->stride];
  u64 mem_location = ret_arr->stride*index;
  // Copy the first half of data into the buffer
  vcopy_memory(buffer, ret_arr->data, mem_location);
  // Copy the item into the buffer
  vcopy_memory(buffer+mem_location, data, ret_arr->stride);
  // Copy the rest of data into the buffer
  vcopy_memory(buffer+(mem_location+ret_arr->stride), 
               ret_arr->data+(mem_location),
               (ret_arr->cap*ret_arr->stride)-mem_location);
  // Now copy the entire buffer back into data
  vcopy_memory(ret_arr->data, buffer, ret_arr->cap*ret_arr->stride);
  ret_arr->length++;
  return ret_arr;
}

darray* darray_remove(darray* arr, u64 index, void* dest){
  if(index >= arr->length){
    VWARN("Attempted to remove from beyond the length of the array");
    return NULL;
  }
  u64 mem_location = arr->stride*index;
  u64 mem_size = (arr->cap*arr->stride)-mem_location-arr->stride;
  u8 buffer[mem_size];
  vcopy_memory(buffer, arr->data+(mem_location+arr->stride), mem_size);
  vcopy_memory(dest, arr->data+mem_location, arr->stride);
  vcopy_memory(arr->data+mem_location, buffer, mem_size);
  arr->length--;
  return arr;
}

darray* darray_delete(darray* arr, u64 index){
  if(index >= arr->length){
    VWARN("Attempted to delete from beyond the length of the array");
    return NULL;
  }
  u64 mem_location = arr->stride*index;
  u64 mem_size = (arr->cap*arr->stride)-mem_location-arr->stride;
  u8 buffer[mem_size];
  vcopy_memory(buffer, arr->data+(mem_location+arr->stride), mem_size);
  vcopy_memory(arr->data+mem_location, buffer, mem_size);
  arr->length--;
  return arr;
}

void darray_free(darray* arr){
  vfree(arr->data, arr->cap*arr->stride, MEMORY_TAG_DARRAY);
  vfree(arr, sizeof(arr), MEMORY_TAG_DARRAY);
}