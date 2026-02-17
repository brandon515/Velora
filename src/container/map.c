#include "map.h"
#include "core/vmemory.h"

int_map* map_new(u64 item_size){
  int_map* ret_map = vallocate(sizeof(int_map), MEMORY_TAG_INT_MAP);
  ret_map->data = darray_new(item_size);
  ret_map->keys = darray_new(sizeof(u64));
  return ret_map;
}

u64 map_get_index(int_map* map, u64 key){
  int key_index = -1;
  u64* map_keys = (u64*)map->keys->data;
  for(int i = 0; i < map->keys->length; i++){
    if(map_keys[i] == key){
      key_index = i;
      break;
    }
  }
  return key_index;
}

b8 map_get_item(int_map* map, u64 key, void* dest){
  int key_index = map_get_index(map, key);
  if(key_index == -1){
    return FALSE;
  }
  vcopy_memory(dest, map->data->data+(map->data->stride*key_index), map->data->stride);
  return TRUE;
}

void map_set_item(int_map* map, u64 key, void* item){
  int key_index = map_get_index(map, key);
  if(key_index == -1){ // The key doesn't exist, just slam it into the end
    map->keys = darray_push(map->keys, &key);
    map->data = darray_push(map->data, item);
  }else{ // The key does exist, replace the current thing with the new thing
    darray_delete(map->data, key_index);
    map->data = darray_insert(map->data, item, key_index);
  }
}

VAPI void map_free(int_map* map){
  darray_free(map->data);
  darray_free(map->keys);
  vfree(map, sizeof(map), MEMORY_TAG_INT_MAP);
}