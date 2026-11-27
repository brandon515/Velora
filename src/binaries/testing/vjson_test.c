#include "vjson_test.h"
#include "test_framework.h"
#include "assets/vjson.h"
#include "core/utils/vstring.h"

#include <string.h>

/*
 * vjson only reads files, so each test writes its JSON to a scratch file in
 * the working directory first. Names start with vt_json_ so they can't clash
 * with the gltf suite when meson runs suites in parallel.
 */

#define JSON_PATH "vt_json_input.json"

static b8 parse(const char* text, json_value* out) {
  *out = (json_value){0};
  if(!write_test_text_file(JSON_PATH, text)) {
    return FALSE;
  }
  b8 ok = import_json_file(JSON_PATH, out);
  remove_test_file(JSON_PATH);
  return ok;
}

static void free_loaded_string(char* s) {
  if(s != NULL) {
    vfree(s, vstrlen(s) + 1, MEMORY_TAG_STRING);
  }
}

static void test_missing_file(void) {
  json_value v = {0};
  CHECK(import_json_file("vt_json_does_not_exist.json", &v) == FALSE, "missing file returns FALSE");
}

static void test_scalars(void) {
  json_value root;
  REQUIRE(parse("{\"int\": 42, \"neg\": -17, \"dbl\": 3.5, \"t\": true, \"f\": false, \"s\": \"hi\"}", &root), "object of scalars parses");
  CHECK(root.type == VELORA_JSON_OBJECT, "root is an object");
  u64 u = 0;
  CHECK(load_json_unsigned_integer(&root, "int", &u) && u == 42, "unsigned integer");
  i64 i = 0;
  CHECK(load_json_signed_integer(&root, "neg", &i) && i == -17, "negative integer");
  f64 d = 0;
  CHECK(load_json_float(&root, "dbl", &d) && d == 3.5, "double");
  CHECK(load_json_float(&root, "int", &d) && d == 42.0, "load_json_float accepts an integer");
  char* s = NULL;
  CHECK(load_json_string(&root, "s", &s) && s != NULL && strcmp(s, "hi") == 0, "string");
  free_loaded_string(s);

  json_value* vals = root.data.objectArray;
  u64 count = root.dataSize / sizeof(json_value);
  b8 saw_true = FALSE, saw_false = FALSE;
  for(u64 k = 0; k < count; k++) {
    if(vals[k].type == VELORA_JSON_BOOL && strcmp(vals[k].name, "t") == 0) {
      saw_true = vals[k].data.boolean == TRUE;
    }
    if(vals[k].type == VELORA_JSON_BOOL && strcmp(vals[k].name, "f") == 0) {
      saw_false = vals[k].data.boolean == FALSE;
    }
  }
  CHECK(saw_true && saw_false, "true and false parse as booleans");
  CHECK(count == 6, "object holds 6 values");
  free_json_value(&root);
}

static void test_getters_reject_wrong_type_or_name(void) {
  json_value root;
  REQUIRE(parse("{\"s\": \"text\", \"n\": 1}", &root), "parses");
  u64 u = 7;
  CHECK(load_json_unsigned_integer(&root, "s", &u) == FALSE, "integer getter rejects a string");
  CHECK(load_json_unsigned_integer(&root, "missing", &u) == FALSE, "integer getter rejects a missing name");
  CHECK(u == 7, "failed getter leaves the output alone");
  char* s = NULL;
  CHECK(load_json_string(&root, "n", &s) == FALSE, "string getter rejects an integer");
  json_value obj;
  CHECK(load_json_object(&root, "n", &obj) == FALSE, "object getter rejects an integer");
  CHECK(load_json_unsigned_integer(NULL, "n", &u) == FALSE, "NULL object rejected");
  CHECK(load_json_unsigned_integer(&root, NULL, &u) == FALSE, "NULL name rejected");
  free_json_value(&root);
}

static void test_string_escapes(void) {
  json_value root;
  REQUIRE(parse("{\"s\": \"a\\\"b\\\\c\\/d\\ne\\tf\"}", &root), "escaped string parses");
  char* s = NULL;
  REQUIRE(load_json_string(&root, "s", &s) == TRUE, "string found");
  CHECK(strcmp(s, "a\"b\\c/d\ne\tf") == 0, "escapes decoded");
  free_loaded_string(s);
  free_json_value(&root);
}

static void test_nested_object(void) {
  json_value root;
  REQUIRE(parse("{\"outer\": {\"inner\": {\"x\": 5}, \"y\": 6}, \"z\": 7}", &root), "nested objects parse");
  json_value outer, inner;
  REQUIRE(load_json_object(&root, "outer", &outer), "outer object found");
  REQUIRE(load_json_object(&outer, "inner", &inner), "inner object found");
  u64 v = 0;
  CHECK(load_json_unsigned_integer(&inner, "x", &v) && v == 5, "value in inner object");
  CHECK(load_json_unsigned_integer(&outer, "y", &v) && v == 6, "value after a nested object");
  CHECK(load_json_unsigned_integer(&root, "z", &v) && v == 7, "value after the outer object");
  free_json_value(&root);
}

static void test_whitespace_and_newlines(void) {
  json_value root;
  REQUIRE(parse("{\n\t\"a\" :\t1 ,\r\n  \"b\":[1,\n 2]\n}\n", &root), "JSON with assorted whitespace parses");
  u64 v = 0;
  CHECK(load_json_unsigned_integer(&root, "a", &v) && v == 1, "value after spaced colon");
  u64* arr = NULL;
  u64 count = 0;
  CHECK(load_json_unsigned_integer_array(&root, "b", &arr, &count) && count == 2, "array split over lines");
  if(arr != NULL) {
    vfree(arr, count * sizeof(u64), MEMORY_TAG_JSON);
  }
  free_json_value(&root);
}

static void test_integer_arrays(void) {
  json_value root;
  REQUIRE(parse("{\"u\": [1, 2, 3], \"i\": [-1, 0, 5]}", &root), "integer arrays parse");
  u64* u = NULL;
  u64 count = 0;
  REQUIRE(load_json_unsigned_integer_array(&root, "u", &u, &count), "unsigned array");
  CHECK(count == 3 && u[0] == 1 && u[1] == 2 && u[2] == 3, "unsigned array values");
  vfree(u, count * sizeof(u64), MEMORY_TAG_JSON);
  i64* i = NULL;
  REQUIRE(load_json_signed_integer_array(&root, "i", &i, &count), "signed array");
  CHECK(count == 3 && i[0] == -1 && i[1] == 0 && i[2] == 5, "signed array values");
  vfree(i, count * sizeof(i64), MEMORY_TAG_JSON);
  free_json_value(&root);
}

static void test_float_array_of_doubles(void) {
  json_value root;
  REQUIRE(parse("{\"f\": [0.5, -1.25, 2.0]}", &root), "float array parses");
  f64* f = NULL;
  u64 count = 0;
  REQUIRE(load_json_float_array(&root, "f", &f, &count), "float array loads");
  CHECK(count == 3 && f[0] == 0.5 && f[1] == -1.25 && f[2] == 2.0, "float array values");
  vfree(f, count * sizeof(f64), MEMORY_TAG_JSON);
  free_json_value(&root);
}

/*
 * Tier 2: load_json_float_array rejects integers, so glTF transforms and
 * colours written as [0,0,0,1] silently fall back to defaults.
 */
static void test_float_array_accepts_integers(void) {
  json_value root;
  REQUIRE(parse("{\"q\": [0, 0, 0, 1], \"mixed\": [1, 2.5]}", &root), "parses");
  f64* f = NULL;
  u64 count = 0;
  CHECK(load_json_float_array(&root, "q", &f, &count) == TRUE, "float array of integers loads");
  if(f != NULL && count == 4) {
    CHECK(f[0] == 0 && f[3] == 1, "integers converted to doubles");
    vfree(f, count * sizeof(f64), MEMORY_TAG_JSON);
  }
  f = NULL;
  CHECK(load_json_float_array(&root, "mixed", &f, &count) == TRUE, "mixed integer/double array loads");
  if(f != NULL && count == 2) {
    CHECK(f[0] == 1 && f[1] == 2.5, "mixed values");
    vfree(f, count * sizeof(f64), MEMORY_TAG_JSON);
  }
  free_json_value(&root);
}

static void test_string_array(void) {
  json_value root;
  REQUIRE(parse("{\"s\": [\"one\", \"two\"]}", &root), "string array parses");
  char** s = NULL;
  u64 count = 0;
  REQUIRE(load_json_string_array(&root, "s", &s, &count), "string array loads");
  CHECK(count == 2 && strcmp(s[0], "one") == 0 && strcmp(s[1], "two") == 0, "string array values");
  for(u64 k = 0; k < count; k++) {
    free_loaded_string(s[k]);
  }
  vfree(s, count * sizeof(char*), MEMORY_TAG_JSON);
  free_json_value(&root);
}

static void test_object_array(void) {
  json_value root;
  REQUIRE(parse("{\"items\": [{\"id\": 1}, {\"id\": 2}, {\"id\": 3}]}", &root), "array of objects parses");
  json_value* items = NULL;
  u64 count = 0;
  REQUIRE(load_json_object_array(&root, "items", &items, &count), "object array loads");
  CHECK(count == 3, "three objects");
  u64 id = 0;
  CHECK(count == 3 && load_json_unsigned_integer(&items[2], "id", &id) && id == 3, "third object's id");
  free_json_value(&root);
}

static void test_empty_containers(void) {
  json_value root;
  REQUIRE(parse("{\"a\": [], \"o\": {}, \"n\": 1}", &root), "empty array and object parse");
  u64 v = 0;
  CHECK(load_json_unsigned_integer(&root, "n", &v) && v == 1, "value after empty containers");
  free_json_value(&root);

  REQUIRE(parse("{}", &root), "empty root object parses");
  free_json_value(&root);
}

// Tier 2: an array holding only whitespace, "[ ]", fails to parse.
static void test_empty_array_with_space(void) {
  json_value root;
  CHECK(parse("{\"a\": [ ], \"n\": 1}", &root) == TRUE, "\"[ ]\" parses as an empty array");
  free_json_value(&root);
}

/*
 * An empty array must report its own length and zero items. Before, it left
 * the caller's length unset: in an object the parser reused the key's
 * length and skipped too far, and inside an array it never advanced and
 * looped forever.
 */
static void test_empty_array_reports_its_length(void) {
  json_value root;
  u64 v = 0;
  REQUIRE(parse("{\"abcdefg\": [], \"n\": 1}", &root), "empty array after a long key parses");
  CHECK(load_json_unsigned_integer(&root, "n", &v) && v == 1, "value after the empty array");
  free_json_value(&root);

  REQUIRE(parse("{\"a\": [[], 1]}", &root), "empty array nested in an array parses");
  json_value* a = &root.data.objectArray[0];
  REQUIRE(a->type == VELORA_JSON_ARRAY && a->dataSize == 2 * sizeof(json_value), "outer array has two elements");
  CHECK(a->data.array[0].type == VELORA_JSON_ARRAY && a->data.array[0].dataSize == 0, "first element is an empty array");
  CHECK(a->data.array[1].type == VELORA_JSON_INTEGER && a->data.array[1].data.integer == 1, "second element is 1");
  free_json_value(&root);

  REQUIRE(parse("{\"a\": [ [ ] , 2 ]}", &root), "spaced empty array nested in an array parses");
  a = &root.data.objectArray[0];
  REQUIRE(a->type == VELORA_JSON_ARRAY && a->dataSize == 2 * sizeof(json_value), "outer array has two elements");
  CHECK(a->data.array[0].type == VELORA_JSON_ARRAY && a->data.array[0].dataSize == 0, "first element is an empty array");
  CHECK(a->data.array[1].type == VELORA_JSON_INTEGER && a->data.array[1].data.integer == 2, "second element is 2");
  free_json_value(&root);
}

// Tier 2: exponents with an explicit + sign don't parse.
static void test_exponent_numbers(void) {
  json_value root;
  REQUIRE(parse("{\"a\": 1e3, \"b\": 2.5E-2}", &root), "exponents parse");
  f64 d = 0;
  CHECK(load_json_float(&root, "a", &d) && d == 1000.0, "1e3");
  CHECK(load_json_float(&root, "b", &d) && d == 0.025, "2.5E-2");
  free_json_value(&root);

  CHECK(parse("{\"c\": 1e+20}", &root) == TRUE, "1e+20 parses");
  if(root.data.objectArray != NULL) {
    CHECK(load_json_float(&root, "c", &d) && d == 1e20, "1e+20 value");
    free_json_value(&root);
  }
}

// Issue #20: a null value in an object skips the wrong number of bytes.
static void test_null_in_object(void) {
  json_value root;
  CHECK(parse("{\"a\": null, \"b\": 5}", &root) == TRUE, "object with a null value parses");
  u64 v = 0;
  CHECK(load_json_unsigned_integer(&root, "b", &v) && v == 5, "value after null");
  free_json_value(&root);
}

static void test_import_then_free_returns_all_memory(void) {
  json_value root;
  u64 before = get_memory_allocated();
  REQUIRE(parse("{\"a\": 1, \"b\": \"two\", \"c\": [1, 2], \"d\": {\"e\": 3.5}}", &root), "parses");
  free_json_value(&root);
  CHECK(get_memory_allocated() == before, "import_json_file + free_json_value frees the file buffer and every name (Tier 3 leak)");
}

/*
 * Issue #20: a null array element never advances the index, so the
 * parser loops until memory runs out. Crash/timeout tests are kept at the
 * end of the suite.
 */
static void test_null_in_array(void) {
  json_value root;
  CHECK(parse("{\"a\": [null, 1]}", &root) == TRUE, "array containing null parses");
  free_json_value(&root);
}

/*
 * A value starting with 'n' that isn't "null" must be rejected, and the
 * parser must free everything it built so far (run with the leak check).
 */
static void test_malformed_null_in_object_is_rejected(void) {
  json_value root;
  CHECK(parse("{\"a\": 1, \"s\": \"x\", \"b\": nope}", &root) == FALSE, "malformed null in object is rejected");
  CHECK(parse("{\"a\": nul", &root) == FALSE, "truncated null in object is rejected");
}

static void test_malformed_null_in_array_is_rejected(void) {
  json_value root;
  CHECK(parse("{\"a\": [1, \"x\", nope]}", &root) == FALSE, "malformed null in array is rejected");
  CHECK(parse("{\"a\": [nul", &root) == FALSE, "truncated null in array is rejected");
}

/*
 * Issue #21: the file buffer has no terminator and the scanners have
 * no end-of-buffer check, so truncated input reads past the buffer.
 */
static void test_truncated_input_fails_cleanly(void) {
  json_value root;
  CHECK(parse("{\"a\": 1, \"b\": \"unterminated", &root) == FALSE, "unterminated string is rejected");
  CHECK(parse("{\"a\": 1", &root) == FALSE, "missing closing brace is rejected");
  CHECK(parse("{\"a\"", &root) == FALSE, "missing colon is rejected");
}

void vjson_test(void) {
  RUN_TEST(test_missing_file);
  RUN_TEST(test_scalars);
  RUN_TEST(test_getters_reject_wrong_type_or_name);
  RUN_TEST(test_string_escapes);
  RUN_TEST(test_nested_object);
  RUN_TEST(test_whitespace_and_newlines);
  RUN_TEST(test_integer_arrays);
  RUN_TEST(test_float_array_of_doubles);
  RUN_TEST(test_float_array_accepts_integers);
  RUN_TEST(test_string_array);
  RUN_TEST(test_object_array);
  RUN_TEST(test_empty_containers);
  RUN_TEST(test_empty_array_with_space);
  RUN_TEST(test_empty_array_reports_its_length);
  RUN_TEST(test_exponent_numbers);
  RUN_TEST(test_null_in_object);
  RUN_TEST(test_import_then_free_returns_all_memory);
  RUN_TEST(test_null_in_array);
  RUN_TEST(test_malformed_null_in_object_is_rejected);
  RUN_TEST(test_malformed_null_in_array_is_rejected);
  RUN_TEST(test_truncated_input_fails_cleanly);
}
