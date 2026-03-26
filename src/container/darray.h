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
  u64 length; // number of elements in data
  u64 stride; // size of a single element in data in bytes
  u64 cap;    // max number of elements for memory allocated in data
} darray;

typedef struct _iterator{
  darray *array;
  u64 curIndex;
}iterator;

/*!
 * @brief Creates a new dynamic array on the heap
 * @param stride  The size of each individual item
 * @result A newly allocated dynamic array on the heap
 */
darray* darray_new(u64 stride);

/*!
 * @brief Copies the data from the pointer given into the end of the dynamic array
 * @param arr Pointer to dynamic array data is being added to
 * @param data  Data pointer to the item being added
 * @result Pointer to the dynamic array
 */
darray* darray_push(darray* arr, const void* data);

/*!
 * @brief Copies the data from end of the darray and decreases the length by one. Doesn't zero out the final item.
 * @param arr Pointer to dynamic array item is popped out of
 * @param dest  Pointer to deposit the data
 * @result Pointer to darray, returns NULL if dynamic array is empty
 */
darray* darray_pop(darray* arr, void* dest);

/*!
 * @brief Places data from data pointer to the index
 * @param arr Pointer to dynamic array
 * @param data  Pointer to data to be inserted
 * @param index Index to place data
 * @result Pointer to darray, returns NULL if trying to insert beyond the length of the array
 */
darray* darray_insert(darray* arr, const void* data, u64 index);

/*!
 * @brief Removes data from index and closes the gap in memory, putting the removed data into dest
 * @param arr Pointer to dynamic array
 * @param index Index to remove
 * @param dest  Pointer to copy data into, this should already be allocated
 * @return Pointer to darray, returns NULL if index is beyond the length of the dynamic array
 */
darray* darray_remove(darray* arr, u64 index, void* dest);

/*!
 * @brief Destroys data at index and closes the gap in memory
 * @param arr Pointer to dynamic array
 * @param index Index to remove
 * @return Pointer to darray, returns NULL if index is beyond the length of the dynamic array
 */
darray* darray_delete(darray* arr, u64 index);

/*!
 * @brief Cleans up the dynamic array by free the data and itself, the dynamic array should not be used beyond this call
 * @param arr Pointer to dynamic array
 */
void darray_free(darray* arr);

/*!
 * @brief Zeros out the memory of the dynamic array while leaving the cap intact
 * @param arr Pointer to dynamic array
 */
void darray_clear(darray* arr);

/*!
 * @brief Deletes the first num_of_items items in the dynamic array in FIFO order
 * @param arr Pointer to dynamic array
 * @param num_of_items Number of items to delete
 * @result Pointer to array, returns NULL if num_of_items is larger than the dynamic array's length
 */
darray* darray_drain(darray* arr, u64 num_of_items);

/*!
 * @brief Gets a pointer to the data in the dynamic array. Making this pointer mutable
 * @param arr Pointer to dynamic array
 * @param index The index of the item in the array
 * @result Pointer to the data in the array. This pointer is not to be kept as it might change if the dynamic array changes. return NULL if the index is larger than the size of the array
 */
void* darray_get_pointer(darray* arr, u64 index);

/*!
 * @brief Gets the data at the index in the dynamic array. The data is copied into the pointer, making this data immutable
 * @param arr Pointer to dynamic array
 * @param index The index of the item in the array
 * @param data A pointer to a stack variable or already allocated heap variable to house the data
 * @result TRUE if data was able to be loaded, FALSE if the data wasn't able to be loaded for any reason, usually this is because the index is larger than the size of the dynamic array
 */
b8 darray_get_data(darray* arr, u64 index, void* data);

/*!
 * @brief Creates an iterator to iterator over the dynamic array
 * @param arr Pointer to dynamic array
 * @result Pass by reference iterator struct for this dynamic array
 */
iterator create_iterator(darray* arr);

/*!
 * @brief Resets to the iterator to the start of the dynamic array
 * @param iter A pointer to the iterator to reset
 */
void iterator_restart(iterator* iter);

/*!
 * @brief Gets a pointer to the next item in the dynamic array and pointer the dataPointer to it
 * @param iter Pointer to the iterator
 * @param dataPointer The index of the item in the array
 * @result TRUE if data was able to be pointer to, FALSE if the data wasn't able to be pointed to, usually because the iterator is already beyond the length of the dynamic array
 */
b8 iterator_next(iterator* iter, void** dataPointer);

/*!
 * @brief Gets the index of the last item returned in the function iterator_next
 * @param iter Pointer to iterator
 * @result The index of the last item returned in iterator next. If iterator is at the final element, returns the final index. U64_MAX if the iterator is resting at the beginning of the dynamic array.
 */
u64 iterator_index_of_last_Item(iterator* iter);