#pragma once

#include "defines.h"
#include "darray.h"

typedef struct _int_map{
  darray* data;
  darray* keys;
} int_map;

VAPI int_map* map_create(u64 item_size);
VAPI u64 map_get(int_map* map, u64 key);
VAPI void map_set(int_map* map, u64 key, void* item);