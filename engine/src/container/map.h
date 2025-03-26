#pragma once

#include "defines.h"
#include "darray.h"

typedef struct _int_map{
  darray* data;
  darray* keys;
} int_map;

/*!
 * @brief Creates a new int_map
 * @param item_size The size of a single item in the map/dynamic array
 * @result Pointer to the new int_map allocated in this function
 */
VAPI int_map* map_new(u64 item_size);

/*!
 * @brief Places data from data pointer to the index
 * @param map Pointer to int_map
 * @param key The key used to locate the data requested
 * @param dest The memory location to copy the item into
 * @result TRUE if the key exists and points to data, FALSE if they do not
 */
VAPI b8 map_get_item(int_map* map, u64 key, void* dest);

/*!
 * @brief Copies data from item into the int_map
 * @param map Pointer to int_map
 * @param key The key used to place the data requested
 * @param item A pointer that the memory is copied out of
 */
VAPI void map_set_item(int_map* map, u64 key, void* item);

/*!
 * @brief Frees the memory of the dynamic arrays and int_map itself, the int_map shouldn't be used past this function call
 * @param map Pointer to int_map to be freed
 */
VAPI void map_free(int_map* map);