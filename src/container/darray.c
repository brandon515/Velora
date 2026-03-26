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
  u64 new_cap = arr->cap*REALLOCATE_MULTIPLIER;
  u64 current_mem_size = arr->cap*arr->stride;
  u64 new_mem_size = new_cap*arr->stride;
  u8 buffer[current_mem_size];
  vcopy_memory(&buffer, arr->data, current_mem_size);
  vfree(arr->data, current_mem_size, MEMORY_TAG_DARRAY);
  arr->data = vallocate(new_mem_size, MEMORY_TAG_DARRAY);
  vcopy_memory(arr->data, &buffer, current_mem_size);
  arr->cap = new_cap;
  return arr;
}

darray* darray_push(darray* arr, const void* data){
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

darray* darray_insert(darray* arr, const void* data, u64 index){
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
  vfree(arr, sizeof(darray), MEMORY_TAG_DARRAY);
}

void darray_clear(darray* arr){
  vzero_memory(arr->data, arr->cap*arr->stride);
  arr->length = 0;
}

darray* darray_drain(darray* arr, u64 num_of_items){
  if(num_of_items > arr->length){
    return NULL;
  }
  u64 mem_location = arr->stride*num_of_items;
  u64 darray_mem_size = arr->stride*arr->cap;
  u64 backend_mem_size = darray_mem_size - mem_location;
  u8 buffer[backend_mem_size];
  vcopy_memory(&buffer, arr->data+mem_location, backend_mem_size);
  vzero_memory(arr->data, darray_mem_size);
  vcopy_memory(arr->data, &buffer, backend_mem_size);
  arr->length -= num_of_items;
  return arr;
}

void* darray_get_pointer(darray* arr, u64 index){
  if(arr->length >= index){
    return NULL;
  }
  return arr->data+(index*arr->stride*index);
}

b8 darray_get_data(darray* arr, u64 index, void* data){
  if(arr->length >= index){
    return FALSE;
  }
  vcopy_memory(data, arr->data+(arr->stride*index), arr->stride);
  return TRUE;
}

iterator darray_create_iterator(darray* arr){
  iterator ret = {
    .array = arr,
    .curIndex = 0,
  };
  return ret;
}

void iterator_restart(iterator* iter){
  iter->curIndex = 0;
}

b8 iterator_next(iterator* iter, void** dataPointer){
  if(iter->curIndex >= iter->array->length){
    return FALSE;
  }
  (*dataPointer) = darray_get_pointer(iter->array, iter->curIndex);
  iter->curIndex = iter->curIndex+1;
  return TRUE;
}

u64 iterator_index_of_last_Item(iterator* iter){
  if(iter->curIndex == 0){
    return U64_MAX;
  }
  return iter->curIndex-1;
}
