#include "vfile_test.h"
#include "test_framework.h"
#include "core/utils/vfile.h"
#include "core/utils/vstring.h"

#include <string.h>

#define FILE_PATH "vt_file_input.bin"

static void free_string(char* s) {
  if(s != NULL) {
    vfree(s, vstrlen(s) + 1, MEMORY_TAG_STRING);
  }
}

static void test_read_file_contents(void) {
  u8 data[300];
  for(u64 i = 0; i < sizeof(data); i++) {
    data[i] = (u8)(i * 7);
  }
  REQUIRE(write_test_file(FILE_PATH, data, sizeof(data)), "write scratch file");
  velora_file file = {0};
  CHECK(get_file_contents(FILE_PATH, &file) == TRUE, "existing file reads");
  CHECK(file.size == sizeof(data), "size matches the file");
  CHECK(file.contents != NULL && memcmp(file.contents, data, sizeof(data)) == 0, "contents match byte for byte");
  free_velora_file(&file);
  CHECK(file.size == 0, "free_velora_file resets the size");
  remove_test_file(FILE_PATH);
}

static void test_read_empty_file(void) {
  REQUIRE(write_test_file(FILE_PATH, "", 0), "write empty scratch file");
  velora_file file = {0};
  CHECK(get_file_contents(FILE_PATH, &file) == TRUE, "empty file reads");
  CHECK(file.size == 0, "empty file has size 0");
  free_velora_file(&file);
  remove_test_file(FILE_PATH);
}

static void test_missing_file(void) {
  velora_file file = {0};
  CHECK(get_file_contents("vt_file_does_not_exist.bin", &file) == FALSE, "missing file returns FALSE");
}

static void test_read_file_text(void) {
  const char* text = "{\"key\": \"value\"}\n";
  u64 len = vstrlen(text);
  REQUIRE(write_test_text_file(FILE_PATH, text), "write scratch text file");
  u64 before = get_memory_allocated();
  velora_file file = {0};
  CHECK(get_file_text(FILE_PATH, &file) == TRUE, "existing text file reads");
  CHECK(file.size == len + 1, "size is the file size plus the terminator");
  CHECK(get_memory_allocated() == before + len + 1, "allocation matches the reported size");
  CHECK(file.contents != NULL && memcmp(file.contents, text, len) == 0, "contents match the file");
  CHECK(file.contents != NULL && file.contents[len] == '\0', "contents are zero terminated");
  CHECK(file.contents != NULL && vstrlen((char*)file.contents) == len, "contents work as a C string");
  free_velora_file(&file);
  CHECK(file.size == 0, "free_velora_file resets the size");
  CHECK(get_memory_allocated() == before, "free_velora_file releases the whole allocation");
  remove_test_file(FILE_PATH);
}

static void test_read_empty_file_text(void) {
  REQUIRE(write_test_file(FILE_PATH, "", 0), "write empty scratch file");
  velora_file file = {0};
  CHECK(get_file_text(FILE_PATH, &file) == TRUE, "empty text file reads");
  CHECK(file.size == 1, "empty text file has only the terminator");
  CHECK(file.contents != NULL && file.contents[0] == '\0', "empty text file is an empty string");
  free_velora_file(&file);
  remove_test_file(FILE_PATH);
}

// Binary files with embedded zeros still read in full; size, not the terminator, gives the length.
static void test_read_file_text_with_embedded_zero(void) {
  const u8 data[] = {'a', 0, 'b'};
  REQUIRE(write_test_file(FILE_PATH, data, sizeof(data)), "write scratch file with an embedded zero");
  velora_file file = {0};
  CHECK(get_file_text(FILE_PATH, &file) == TRUE, "file with an embedded zero reads");
  CHECK(file.size == sizeof(data) + 1, "size covers every byte plus the terminator");
  CHECK(file.contents != NULL && memcmp(file.contents, data, sizeof(data)) == 0 && file.contents[sizeof(data)] == '\0',
        "every byte is kept and the terminator follows");
  free_velora_file(&file);
  remove_test_file(FILE_PATH);
}

static void test_missing_file_text(void) {
  velora_file file = {0};
  CHECK(get_file_text("vt_file_does_not_exist.txt", &file) == FALSE, "missing text file returns FALSE");
  CHECK(file.contents == NULL && file.size == 0, "missing text file leaves the output empty");
}

static void test_get_file_path(void) {
  char* path = get_file_path("models/cube/cube.gltf");
  CHECK(path != NULL && strcmp(path, "models/cube/") == 0, "forward-slash path keeps the trailing slash");
  free_string(path);

  path = get_file_path("models\\cube.gltf");
  CHECK(path != NULL && strcmp(path, "models\\") == 0, "backslash path");
  free_string(path);

  path = get_file_path("/abs.gltf");
  CHECK(path != NULL && strcmp(path, "/") == 0, "file in the root directory");
  free_string(path);
}

// Tier 2 (via vsubstr): a URI that ends in a slash is its own directory, but vsubstr returns NULL.
static void test_get_file_path_of_directory(void) {
  char* path = get_file_path("models/");
  CHECK(path != NULL, "directory URI gives a path");
  if(path != NULL) {
    CHECK(strcmp(path, "models/") == 0, "directory URI is returned unchanged");
  }
  free_string(path);
}

/*
 * Tier 3: for a bare file name get_file_path returns the string literal
 * "./", which callers can't tell apart from a heap string they must free.
 * Whatever it returns for a bare name must either be NULL (as the header
 * documents) or a heap string the caller owns.
 */
static void test_get_file_path_of_bare_name_is_freeable(void) {
  u64 before = get_memory_allocated();
  char* path = get_file_path("cube.gltf");
  b8 owned = path == NULL || get_memory_allocated() == before + vstrlen(path) + 1;
  CHECK(owned, "bare file name gives NULL or a heap-allocated string");
  if(path != NULL && owned) {
    free_string(path);
  }
}

void vfile_test(void) {
  RUN_TEST(test_read_file_contents);
  RUN_TEST(test_read_empty_file);
  RUN_TEST(test_missing_file);
  RUN_TEST(test_read_file_text);
  RUN_TEST(test_read_empty_file_text);
  RUN_TEST(test_read_file_text_with_embedded_zero);
  RUN_TEST(test_missing_file_text);
  RUN_TEST(test_get_file_path);
  RUN_TEST(test_get_file_path_of_directory);
  RUN_TEST(test_get_file_path_of_bare_name_is_freeable);
}
