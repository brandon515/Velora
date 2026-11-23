#include "map_test.h"
#include "test_framework.h"
#include "core/container/map.h"

static void test_set_then_get(void) {
  int_map* map = map_new(sizeof(u64));
  u64 v = 42;
  map_set_item(map, 7, &v);
  u64 out = 0;
  CHECK(map_get_item(map, 7, &out) == TRUE, "get finds a key that was set");
  CHECK(out == 42, "get returns the stored value");
  map_free(map);
}

static void test_get_missing_key(void) {
  int_map* map = map_new(sizeof(u64));
  u64 out = 99;
  CHECK(map_get_item(map, 7, &out) == FALSE, "get on an empty map returns FALSE");
  u64 v = 1;
  map_set_item(map, 3, &v);
  CHECK(map_get_item(map, 4, &out) == FALSE, "get on a missing key returns FALSE");
  CHECK(out == 99, "failed get leaves dest untouched");
  map_free(map);
}

static void test_many_keys(void) {
  int_map* map = map_new(sizeof(u64));
  for(u64 k = 0; k < 50; k++) {
    u64 v = k * 1000 + 1;
    map_set_item(map, k * 3, &v);
  }
  b8 all_found = TRUE;
  for(u64 k = 0; k < 50; k++) {
    u64 out = 0;
    all_found = all_found && map_get_item(map, k * 3, &out) && out == k * 1000 + 1;
  }
  CHECK(all_found, "every one of 50 keys maps to its own value");
  u64 out = 0;
  CHECK(map_get_item(map, 1, &out) == FALSE, "a key between stored keys is missing");
  map_free(map);
}

static void test_replace_first_key(void) {
  int_map* map = map_new(sizeof(u64));
  for(u64 k = 0; k < 3; k++) {
    u64 v = k;
    map_set_item(map, k, &v);
  }
  u64 v = 100;
  map_set_item(map, 0, &v);
  u64 out = 0;
  CHECK(map_get_item(map, 0, &out) && out == 100, "replaced first key holds the new value");
  CHECK(map_get_item(map, 1, &out) && out == 1, "key 1 unchanged");
  CHECK(map_get_item(map, 2, &out) && out == 2, "key 2 unchanged");
  CHECK(map->data->length == 3, "replace does not grow the map");
  map_free(map);
}

static void test_replace_middle_key(void) {
  int_map* map = map_new(sizeof(u64));
  for(u64 k = 0; k < 3; k++) {
    u64 v = k;
    map_set_item(map, k, &v);
  }
  u64 v = 100;
  map_set_item(map, 1, &v);
  u64 out = 0;
  CHECK(map_get_item(map, 0, &out) && out == 0, "key 0 unchanged");
  CHECK(map_get_item(map, 1, &out) && out == 100, "replaced middle key holds the new value");
  CHECK(map_get_item(map, 2, &out) && out == 2, "key 2 unchanged");
  map_free(map);
}

// bug-report #4: replacing the last key went through darray_insert at
// index == length, which used to fail, set map->data to NULL and leak.
static void test_replace_last_key(void) {
  int_map* map = map_new(sizeof(u64));
  for(u64 k = 0; k < 3; k++) {
    u64 v = k;
    map_set_item(map, k, &v);
  }
  u64 v = 100;
  map_set_item(map, 2, &v);
  REQUIRE(map->data != NULL, "map data survives replacing the last key");
  u64 out = 0;
  CHECK(map_get_item(map, 2, &out) && out == 100, "replaced last key holds the new value");
  CHECK(map_get_item(map, 0, &out) && out == 0, "key 0 unchanged");
  CHECK(map->data->length == 3, "replace does not grow the map");
  map_free(map);
}

static void test_replace_only_key_repeatedly(void) {
  int_map* map = map_new(sizeof(u64));
  for(u64 i = 0; i < 10; i++) {
    u64 v = i;
    map_set_item(map, 5, &v);
  }
  REQUIRE(map->data != NULL, "map data survives repeated replacement");
  u64 out = 0;
  CHECK(map_get_item(map, 5, &out) && out == 9, "last write wins");
  CHECK(map->data->length == 1 && map->keys->length == 1, "map still holds one entry");
  map_free(map);
}

static void test_struct_values(void) {
  typedef struct {
    u64 a;
    f64 b;
    u8 c[13];
  } payload;
  int_map* map = map_new(sizeof(payload));
  payload p1 = {1, 1.5, "hello"};
  payload p2 = {2, 2.5, "world"};
  map_set_item(map, 10, &p1);
  map_set_item(map, 20, &p2);
  payload out = {0};
  CHECK(map_get_item(map, 20, &out) && out.a == 2 && out.b == 2.5 && out.c[0] == 'w', "struct value round-trips");
  map_free(map);
}

void map_test(void) {
  RUN_TEST(test_set_then_get);
  RUN_TEST(test_get_missing_key);
  RUN_TEST(test_many_keys);
  RUN_TEST(test_replace_first_key);
  RUN_TEST(test_replace_middle_key);
  RUN_TEST(test_replace_last_key);
  RUN_TEST(test_replace_only_key_repeatedly);
  RUN_TEST(test_struct_values);
}
