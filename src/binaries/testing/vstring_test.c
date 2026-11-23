#include "vstring_test.h"
#include "test_framework.h"
#include "core/utils/vstring.h"

#include <string.h>

static void free_string(char* str) {
  if(str != NULL) {
    vfree(str, vstrlen(str) + 1, MEMORY_TAG_STRING);
  }
}

static void test_strlen(void) {
  CHECK(vstrlen("") == 0, "empty string has length 0");
  CHECK(vstrlen("a") == 1, "one character");
  CHECK(vstrlen("hello world") == 11, "length of hello world");
}

static void test_strcmp(void) {
  CHECK(vstrcmp("abc", "abc") == TRUE, "equal strings");
  CHECK(vstrcmp("", "") == TRUE, "two empty strings");
  CHECK(vstrcmp("abc", "abd") == FALSE, "differ in last character");
  CHECK(vstrcmp("abc", "xbc") == FALSE, "differ in first character");
  CHECK(vstrcmp("abc", "abcd") == FALSE, "prefix is not equal");
  CHECK(vstrcmp("abcd", "abc") == FALSE, "longer string is not equal");
  CHECK(vstrcmp("", "a") == FALSE, "empty is not equal to non-empty");
}

static void test_substr_middle(void) {
  char* sub = vsubstr("hello world", 6, 3);
  REQUIRE(sub != NULL, "substring in the middle");
  CHECK(strcmp(sub, "wor") == 0, "substring holds the right characters");
  free_string(sub);

  sub = vsubstr("hello world", 0, 5);
  REQUIRE(sub != NULL, "substring at the start");
  CHECK(strcmp(sub, "hello") == 0, "prefix substring");
  free_string(sub);
}

// Tier 2: vsubstr uses >= so a substring ending exactly at the end of the string returns NULL.
static void test_substr_to_end_of_string(void) {
  char* sub = vsubstr("hello world", 6, 5);
  CHECK(sub != NULL, "substring that ends at the end of the string");
  if(sub != NULL) {
    CHECK(strcmp(sub, "world") == 0, "suffix substring");
  }
  free_string(sub);

  sub = vsubstr("abc", 0, 3);
  CHECK(sub != NULL, "substring that is the whole string");
  if(sub != NULL) {
    CHECK(strcmp(sub, "abc") == 0, "whole-string substring");
  }
  free_string(sub);
}

static void test_substr_out_of_range(void) {
  CHECK(vsubstr("abc", 3, 1) == NULL, "start at length returns NULL");
  CHECK(vsubstr("abc", 1, 5) == NULL, "length past the end returns NULL");
  CHECK(vsubstr("abc", 10, 0) == NULL, "start past the end returns NULL");
}

static void test_concat(void) {
  char* s = vconcat("foo", "bar");
  REQUIRE(s != NULL, "concat returns a string");
  CHECK(strcmp(s, "foobar") == 0, "concat joins the strings");
  free_string(s);

  s = vconcat("", "bar");
  CHECK(strcmp(s, "bar") == 0, "concat with empty first string");
  free_string(s);

  s = vconcat("foo", "");
  CHECK(strcmp(s, "foo") == 0, "concat with empty second string");
  free_string(s);
}

static void test_find(void) {
  CHECK(vfind("a/b/c", '/') == 1, "vfind returns the first match");
  CHECK(vfind("abc", 'a') == 0, "vfind at index 0");
  CHECK(vfind("abc", 'z') == U64_MAX, "vfind with no match returns U64_MAX");
  CHECK(vfind("", 'a') == U64_MAX, "vfind on an empty string");
}

static void test_rfind(void) {
  CHECK(vrfind("a/b/c", '/') == 3, "vrfind returns the last match");
  CHECK(vrfind("abc", 'a') == 0, "vrfind at index 0");
  CHECK(vrfind("abc", 'c') == 2, "vrfind at the last index");
  CHECK(vrfind("abc", 'z') == U64_MAX, "vrfind with no match returns U64_MAX");
  CHECK(vrfind("", 'a') == U64_MAX, "vrfind on an empty string");
}

// Tier 2: vstrtoint never computes a value (it reads str[index] instead of str[i]).
static void test_strtoint(void) {
  i64 v = 999;
  CHECK(vstrtoint("123", &v) == TRUE, "parse 123");
  CHECK(v == 123, "123 parses to 123");

  v = 999;
  CHECK(vstrtoint("-45", &v) == TRUE, "parse -45");
  CHECK(v == -45, "-45 parses to -45");

  v = 999;
  CHECK(vstrtoint("0", &v) == TRUE, "parse 0");
  CHECK(v == 0, "0 parses to 0");

  v = 999;
  CHECK(vstrtoint("42abc", &v) == TRUE, "parse leading digits");
  CHECK(v == 42, "parsing stops at the first non-digit");

  CHECK(vstrtoint("abc", &v) == FALSE, "non-number returns FALSE");
  CHECK(vstrtoint("-", &v) == FALSE, "lone minus returns FALSE");
  CHECK(vstrtoint("", &v) == FALSE, "empty string returns FALSE");
}

static void check_inttostr(i64 in, const char* expected) {
  char* s = NULL;
  CHECK(vinttostr(in, &s) == TRUE, "vinttostr succeeds");
  if(s == NULL) {
    return;
  }
  if(strcmp(s, expected) != 0) {
    VERROR("    vinttostr(%lld) gave \"%s\", expected \"%s\"", in, s, expected);
  }
  CHECK(strcmp(s, expected) == 0, "vinttostr gives the decimal string");
  free_string(s);
}

static void test_inttostr(void) {
  check_inttostr(0, "0");
  check_inttostr(7, "7");
  check_inttostr(12345, "12345");
  check_inttostr(-987, "-987");
  check_inttostr(1000000, "1000000");
  check_inttostr(9223372036854775807LL, "9223372036854775807");
}

// Not in the bug report: negating INT64_MIN overflows (undefined behaviour, caught by UBSan).
static void test_inttostr_int64_min(void) {
  check_inttostr((-9223372036854775807LL) - 1, "-9223372036854775808");
}

void vstring_test(void) {
  RUN_TEST(test_strlen);
  RUN_TEST(test_strcmp);
  RUN_TEST(test_substr_middle);
  RUN_TEST(test_substr_to_end_of_string);
  RUN_TEST(test_substr_out_of_range);
  RUN_TEST(test_concat);
  RUN_TEST(test_find);
  RUN_TEST(test_rfind);
  RUN_TEST(test_strtoint);
  RUN_TEST(test_inttostr);
  RUN_TEST(test_inttostr_int64_min);
}
