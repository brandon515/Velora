#include "darray_test.h"
#include "test_framework.h"
#include "core/container/darray.h"

// Builds an array holding 0, 1, ..., count-1 as u64s.
static darray* make_sequence(u64 count) {
  darray* arr = darray_new(sizeof(u64));
  for(u64 i = 0; i < count; i++) {
    arr = darray_push(arr, &i);
  }
  return arr;
}

// TRUE if arr holds exactly the values in expected, read through darray_get_data.
static b8 contents_equal(darray* arr, const u64* expected, u64 count) {
  if(arr->length != count) {
    VERROR("    length is %llu, expected %llu", arr->length, count);
    return FALSE;
  }
  for(u64 i = 0; i < count; i++) {
    u64 value = 0;
    if(!darray_get_data(arr, i, &value) || value != expected[i]) {
      VERROR("    index %llu holds %llu, expected %llu", i, value, expected[i]);
      return FALSE;
    }
  }
  return TRUE;
}

static void test_push_grows_and_keeps_order(void) {
  darray* arr = make_sequence(10);
  CHECK(arr->length == 10, "length after 10 pushes");
  CHECK(arr->cap >= 10, "cap grew to fit 10 items");
  u64 expected[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CHECK(contents_equal(arr, expected, 10), "pushed values survive reallocation");
  darray_free(arr);
}

static void test_get_pointer_every_index(void) {
  darray* arr = make_sequence(8);
  for(u64 i = 0; i < 8; i++) {
    u64* p = darray_get_pointer(arr, i);
    REQUIRE(p != NULL, "get_pointer returned NULL for an in-range index");
    CHECK(*p == i, "get_pointer points at the right element");
  }
  darray_free(arr);
}

static void test_get_out_of_range(void) {
  darray* arr = make_sequence(3);
  u64 value = 12345;
  CHECK(darray_get_pointer(arr, 3) == NULL, "get_pointer at length returns NULL");
  CHECK(darray_get_data(arr, 3, &value) == FALSE, "get_data at length returns FALSE");
  CHECK(value == 12345, "failed get_data leaves dest untouched");
  darray_free(arr);
}

static void test_pop(void) {
  darray* arr = make_sequence(4);
  u64 value = 0;
  CHECK(darray_pop(arr, &value) != NULL, "pop on non-empty array succeeds");
  CHECK(value == 3, "pop returns the last element");
  CHECK(arr->length == 3, "pop shrinks length");

  darray* empty = darray_new(sizeof(u64));
  CHECK(darray_pop(empty, &value) == NULL, "pop on empty array returns NULL");
  darray_free(empty);
  darray_free(arr);
}

static void test_insert_middle(void) {
  darray* arr = make_sequence(4);
  u64 v = 99;
  arr = darray_insert(arr, &v, 2);
  REQUIRE(arr != NULL, "insert in the middle succeeds");
  u64 expected[5] = {0, 1, 99, 2, 3};
  CHECK(contents_equal(arr, expected, 5), "insert shifts later elements right");
  darray_free(arr);
}

// Exercises the insert path while the array is still small (cap < 8), which is
// where the copy size in darray_insert can run past its stack buffer.
static void test_insert_front_small_array(void) {
  darray* arr = make_sequence(2);
  u64 v = 99;
  arr = darray_insert(arr, &v, 0);
  REQUIRE(arr != NULL, "insert at index 0 succeeds");
  u64 expected[3] = {99, 0, 1};
  CHECK(contents_equal(arr, expected, 3), "insert at front shifts everything right");
  darray_free(arr);
}

// index == length is "at" the end, not "beyond" it, so it should append.
// map_set_item relies on this when replacing the last key (bug-report #4).
static void test_insert_at_length_appends(void) {
  darray* arr = make_sequence(3);
  u64 v = 99;
  darray* result = darray_insert(arr, &v, 3);
  CHECK(result != NULL, "insert at index == length succeeds");
  if(result != NULL) {
    arr = result;
    u64 expected[4] = {0, 1, 2, 99};
    CHECK(contents_equal(arr, expected, 4), "insert at length appends");
  }
  darray_free(arr);
}

static void test_insert_beyond_length_fails(void) {
  darray* arr = make_sequence(3);
  u64 v = 99;
  CHECK(darray_insert(arr, &v, 4) == NULL, "insert past length returns NULL");
  CHECK(arr->length == 3, "failed insert leaves length alone");
  darray_free(arr);
}

static void test_remove_middle(void) {
  darray* arr = make_sequence(5);
  u64 removed = 0;
  CHECK(darray_remove(arr, 2, &removed) != NULL, "remove in range succeeds");
  CHECK(removed == 2, "remove returns the removed element");
  u64 expected[4] = {0, 1, 3, 4};
  CHECK(contents_equal(arr, expected, 4), "remove closes the gap");
  CHECK(darray_remove(arr, 4, &removed) == NULL, "remove at length returns NULL");
  darray_free(arr);
}

static void test_delete(void) {
  darray* arr = make_sequence(5);
  CHECK(darray_delete(arr, 0) != NULL, "delete first succeeds");
  CHECK(darray_delete(arr, arr->length - 1) != NULL, "delete last succeeds");
  u64 expected[3] = {1, 2, 3};
  CHECK(contents_equal(arr, expected, 3), "delete closes the gap");
  CHECK(darray_delete(arr, 3) == NULL, "delete at length returns NULL");
  darray_free(arr);
}

static void test_drain(void) {
  darray* arr = make_sequence(6);
  CHECK(darray_drain(arr, 2) != NULL, "drain 2 succeeds");
  u64 expected[4] = {2, 3, 4, 5};
  CHECK(contents_equal(arr, expected, 4), "drain removes from the front");
  CHECK(darray_drain(arr, 5) == NULL, "drain more than length returns NULL");
  CHECK(darray_drain(arr, 4) != NULL, "drain everything succeeds");
  CHECK(arr->length == 0, "drain everything empties the array");
  darray_free(arr);
}

static void test_clear(void) {
  darray* arr = make_sequence(5);
  u64 cap = arr->cap;
  darray_clear(arr);
  CHECK(arr->length == 0, "clear empties the array");
  CHECK(arr->cap == cap, "clear keeps the capacity");
  darray_free(arr);
}

static void test_iterator(void) {
  darray* arr = make_sequence(6);
  iterator it = darray_create_iterator(arr);
  CHECK(iterator_index_of_last_Item(&it) == U64_MAX, "fresh iterator has no last item");
  u64* p = NULL;
  u64 seen = 0;
  while(iterator_next(&it, (void**)&p)) {
    CHECK(p != NULL && *p == seen, "iterator visits elements in order");
    CHECK(iterator_index_of_last_Item(&it) == seen, "index_of_last_Item tracks position");
    seen++;
  }
  CHECK(seen == 6, "iterator visits every element");
  iterator_restart(&it);
  CHECK(iterator_next(&it, (void**)&p) && *p == 0, "restart goes back to the first element");
  darray_free(arr);
}

static void test_large_stride(void) {
  typedef struct {
    u64 a;
    u64 b;
    u64 c;
  } triple;
  darray* arr = darray_new(sizeof(triple));
  for(u64 i = 0; i < 5; i++) {
    triple t = {i, i * 10, i * 100};
    arr = darray_push(arr, &t);
  }
  triple* t3 = darray_get_pointer(arr, 3);
  REQUIRE(t3 != NULL, "get_pointer on struct array");
  CHECK(t3->a == 3 && t3->b == 30 && t3->c == 300, "struct element read back intact");
  triple removed = {0};
  CHECK(darray_remove(arr, 1, &removed) != NULL, "remove on struct array");
  CHECK(removed.a == 1 && removed.b == 10 && removed.c == 100, "removed struct intact");
  darray_free(arr);
}


// Inserting into a full array has to grow it first.
static void test_insert_grows_full_array(void) {
  darray* arr = darray_new(sizeof(u64));
  u64 expected[64];
  for(u64 i = 0; i < 64; i++) {
    u64 v = 63 - i;
    arr = darray_insert(arr, &v, 0);
    REQUIRE(arr != NULL, "insert at front of a growing array succeeds");
    expected[i] = i;
  }
  CHECK(contents_equal(arr, expected, 64), "64 front inserts give 0..63 in order");
  darray_free(arr);
}

// Insert / remove / delete / drain mixed together on one array.
static void test_mixed_operations(void) {
  darray* arr = make_sequence(10);
  u64 v = 100;
  arr = darray_insert(arr, &v, 5);   // 0 1 2 3 4 100 5 6 7 8 9
  darray_delete(arr, 0);             // 1 2 3 4 100 5 6 7 8 9
  u64 removed = 0;
  darray_remove(arr, 9, &removed);   // 1 2 3 4 100 5 6 7 8
  CHECK(removed == 9, "remove last returns it");
  darray_drain(arr, 2);              // 3 4 100 5 6 7 8
  v = 200;
  arr = darray_push(arr, &v);        // 3 4 100 5 6 7 8 200
  u64 expected[8] = {3, 4, 100, 5, 6, 7, 8, 200};
  CHECK(contents_equal(arr, expected, 8), "mixed operations keep the right order");
  darray_free(arr);
}

/*
 * bug-report #5: every darray function copies the whole array through a
 * stack buffer of cap*stride bytes, so a big array overflows the stack.
 * 12 elements of 1 MiB means copying 8+ MiB through the stack when the
 * array grows, more than the default 8 MiB Linux (or 1 MiB Windows) stack.
 */
static void test_large_array_does_not_overflow_stack(void) {
  const u64 stride = 1024 * 1024;
  const u64 count = 12;
  u8* element = vallocate(stride, MEMORY_TAG_APPLICATION);
  darray* arr = darray_new(stride);
  for(u64 i = 0; i < count; i++) {
    vset_memory(element, (i32)i, stride);
    arr = darray_push(arr, element);
  }
  CHECK(arr->length == count, "pushed 12 one-MiB elements");
  u8* third = darray_get_pointer(arr, 3);
  REQUIRE(third != NULL, "get_pointer on a large array");
  CHECK(third[0] == 3 && third[stride - 1] == 3, "large element read back intact");

  CHECK(darray_remove(arr, 0, element) != NULL, "remove from a large array");
  CHECK(element[0] == 0, "remove returned element 0");
  CHECK(darray_delete(arr, 0) != NULL, "delete from a large array");
  CHECK(darray_drain(arr, 1) != NULL, "drain from a large array");
  u8* first = darray_get_pointer(arr, 0);
  CHECK(first != NULL && first[0] == 3, "element 3 is first after removing three");

  darray_free(arr);
  vfree(element, stride, MEMORY_TAG_APPLICATION);
}

void darray_test(void) {
  RUN_TEST(test_push_grows_and_keeps_order);
  RUN_TEST(test_get_pointer_every_index);
  RUN_TEST(test_get_out_of_range);
  RUN_TEST(test_pop);
  RUN_TEST(test_insert_middle);
  RUN_TEST(test_insert_front_small_array);
  RUN_TEST(test_insert_at_length_appends);
  RUN_TEST(test_insert_beyond_length_fails);
  RUN_TEST(test_remove_middle);
  RUN_TEST(test_delete);
  RUN_TEST(test_drain);
  RUN_TEST(test_clear);
  RUN_TEST(test_iterator);
  RUN_TEST(test_large_stride);
  RUN_TEST(test_insert_grows_full_array);
  RUN_TEST(test_mixed_operations);
  RUN_TEST(test_large_array_does_not_overflow_stack);
}
