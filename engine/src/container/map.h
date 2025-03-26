#pragma once

#include "defines.h"
#include "darray.h"

typedef struct _int_map{
  darray* data;
  darray* keys;
} int_map;

VAPI int_map* map_new(u64 item_size);
VAPI b8 map_get_item(int_map* map, u64 key, void* dest);
VAPI void map_set_item(int_map* map, u64 key, void* item);
VAPI void map_free(int_map* map);