#pragma once

#include "defines.h"
#include "core/logger.h"
#include "core/memory/vmemory.h"

/*
 * Minimal test harness.
 *
 * CHECK records a failure and keeps going, so one run reports every broken
 * expectation instead of stopping at the first. Each suite is a plain
 * function; test_entry.c runs the suite named on the command line and
 * returns non-zero if any CHECK failed, which is all meson test / CI needs.
 *
 * On Linux every test case runs in its own forked child process with a
 * timeout (see test_framework.c). A test that crashes, trips ASan/UBSan, or
 * hangs on a known bug is then reported as one failure and the rest of the
 * suite still runs. On Windows test cases run in-process.
 */

extern u64 g_test_failures;
extern u64 g_test_checks;

#define CHECK(expr, msg) \
  do { \
    g_test_checks++; \
    if(!(expr)) { \
      g_test_failures++; \
      VERROR("FAIL %s:%d: %s (%s)", __FILE__, __LINE__, msg, #expr); \
    } \
  } while(0)

/*
 * Stops the current test case when a CHECK precondition would make the rest
 * of it unsafe (e.g. a NULL pointer that later lines dereference).
 */
#define REQUIRE(expr, msg) \
  do { \
    g_test_checks++; \
    if(!(expr)) { \
      g_test_failures++; \
      VERROR("FAIL %s:%d: %s (%s)", __FILE__, __LINE__, msg, #expr); \
      return; \
    } \
  } while(0)

/*
 * CHECK for floating point values: passes when |actual - expected| <= eps.
 */
#define CHECK_NEAR(actual, expected, eps, msg) \
  do { \
    f64 _a = (f64)(actual); \
    f64 _e = (f64)(expected); \
    f64 _d = _a > _e ? _a - _e : _e - _a; \
    g_test_checks++; \
    if(!(_d <= (eps))) { \
      g_test_failures++; \
      VERROR("FAIL %s:%d: %s (%s = %f, expected %f)", __FILE__, __LINE__, msg, #actual, _a, _e); \
    } \
  } while(0)

/*
 * Runs one test case. When check_leaks is TRUE it also checks the test
 * returned every byte it took from vmemory.
 */
void run_test_case(const char* name, void (*fn)(void), b8 check_leaks);

/* Runs one test case and checks it returned every byte it took from vmemory. */
#define RUN_TEST(fn) run_test_case(#fn, fn, TRUE)

/*
 * Runs one test case without the vmemory leak check. Only for tests whose
 * subject leaks through a known, separately tested bug, so that the test
 * still reports on the behaviour it is about. Switch back to RUN_TEST once
 * that leak is fixed.
 */
#define RUN_TEST_IGNORE_LEAKS(fn) run_test_case(#fn, fn, FALSE)

/*
 * Writes size bytes of data to path, replacing any existing file.
 * Returns FALSE if the file could not be written.
 */
b8 write_test_file(const char* path, const void* data, u64 size);

/* write_test_file for a zero-terminated string (the terminator is not written). */
b8 write_test_text_file(const char* path, const char* text);

/* Deletes a file made by write_test_file. Missing files are ignored. */
void remove_test_file(const char* path);
