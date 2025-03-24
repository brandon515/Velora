#pragma once

#include "defines.h"

#define STARTING_LENGTH 1
#define REALLOCATE_MULTIPLIER 2 // how much to increase memory allocation, 2 is doubling the memory

typedef struct _darray{
  void* data;
  u64 length, // number of elements in data
    stride,   // size of a single element in data in bytes
    cap;      // max number of elements for memory allocated in data
} darray;

/* Creates a new dynamic array on the heap
 * @param The size of each individual item
 * @return A newly allocated dynamic array on the heap
 */
VAPI darray* darray_new(u64 stride);

/* Copies the data from the pointer given into the end of the dynamic array
 * @param Pointer to dynamic array data is being added to
 * @param Data pointer to the item being added
 * @return Pointer to the dynamic array
 */
VAPI darray* darray_push(darray* arr, void* data);

/* Copies the data from end of the darray and decreases the length by one. Doesn't zero out the final item.
 * @param Pointer to dynamic array item is popped out of
 * @param Pointer to deposit the data
 * @return Pointer to darray, returns NULL if dynamic array is empty
 */
VAPI darray* darray_pop(darray* arr, void* dest);

/* Places data from data pointer to the index
 * @param Pointer to dynamic array
 * @param Pointer to data to be inserted
 * @param Index to place data
 * @return Pointer to darray, returns NULL if trying to insert beyond the length of the array
 */
VAPI darray* darray_insert(darray* arr, void* data, u64 index);

/* Removes data from index and closes the gap
 * @param Pointer to dynamic array
 * @param Index to remove
 * @param Pointer to copy data into
 * @return Pointer to darray, returns NULL if index is beyond the length of the dynamic array
 */
VAPI darray* darray_remove(darray* arr, u64 index, void* dest);