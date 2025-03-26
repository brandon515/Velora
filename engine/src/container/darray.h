#pragma once

#include "defines.h"

#define STARTING_LENGTH 1
#define REALLOCATE_MULTIPLIER 2 // how much to increase memory allocation, 2 is doubling the memory

/*!
 * @struct darray darray.h "container/darray.h"
 * @brief Dynamically allocated array that expands when needed
 * @property void* data A pointer to the actual data
 * @property u64 length The number of elements in the data
 * @property u64 stride The size of an individual element
 * @property u64 cap The max number of elements for the memory allocated so far
 */
typedef struct _darray{
  void* data;
  u64 length, // number of elements in data
    stride,   // size of a single element in data in bytes
    cap;      // max number of elements for memory allocated in data
} darray;

/*!
 * @brief Creates a new dynamic array on the heap
 * @param stride  The size of each individual item
 * @result A newly allocated dynamic array on the heap
 */
VAPI darray* darray_new(u64 stride);

/*!
 * @brief Copies the data from the pointer given into the end of the dynamic array
 * @param arr Pointer to dynamic array data is being added to
 * @param data  Data pointer to the item being added
 * @result Pointer to the dynamic array
 */
VAPI darray* darray_push(darray* arr, void* data);

/*!
 * @brief Copies the data from end of the darray and decreases the length by one. Doesn't zero out the final item.
 * @param arr Pointer to dynamic array item is popped out of
 * @param dest  Pointer to deposit the data
 * @result Pointer to darray, returns NULL if dynamic array is empty
 */
VAPI darray* darray_pop(darray* arr, void* dest);

/*!
 * @brief Places data from data pointer to the index
 * @param arr Pointer to dynamic array
 * @param data  Pointer to data to be inserted
 * @param index Index to place data
 * @result Pointer to darray, returns NULL if trying to insert beyond the length of the array
 */
VAPI darray* darray_insert(darray* arr, void* data, u64 index);

/*!
 * @brief Removes data from index and closes the gap in memory, putting the removed data into dest
 * @param arr Pointer to dynamic array
 * @param index Index to remove
 * @param dest  Pointer to copy data into, this should already be allocated
 * @return Pointer to darray, returns NULL if index is beyond the length of the dynamic array
 */
VAPI darray* darray_remove(darray* arr, u64 index, void* dest);

/*!
 * @brief Destroys data at index and closes the gap in memory
 * @param arr Pointer to dynamic array
 * @param index Index to remove
 * @return Pointer to darray, returns NULL if index is beyond the length of the dynamic array
 */
VAPI darray* darray_delete(darray* arr, u64 index);

/*!
 * @brief Cleans up the dynamic array by free the data and itself, the dynamic array should not be used beyond this call
 * @param arr Pointer to dynamic array
 */
VAPI void darray_free(darray* arr);