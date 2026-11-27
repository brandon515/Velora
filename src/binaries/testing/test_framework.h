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

// Name of the suite that is running, for the summary. Set by test_entry.c.
extern const char* g_test_suite;

// --failures: print only the tests that failed (with their output) and the
// summary. Output is captured per test on Linux only.
extern b8 g_test_failures_only;

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
 * Runs one test case that fails today because of the open GitHub issue
 * `issue`. Write the test as if the bug were fixed.
 *
 * - The test fails: reported as XFAIL and doesn't fail the suite.
 * - The test passes: reported as XPASS and fails the suite. The bug is fixed,
 *   so switch the test to RUN_TEST and close the issue.
 *
 * Known-bug tests only run on Linux, where each one is isolated in a forked
 * process. Elsewhere they are reported as SKIP, because a crash in-process
 * would stop the whole suite. CI's known-bugs job checks that every issue
 * named in one is still open.
 */
#define RUN_TEST_KNOWN_BUG(fn, issue) run_known_bug_case(#fn, fn, issue, 0)

/* RUN_TEST_KNOWN_BUG with any of the KNOWN_BUG_* flags below, OR-ed together. */
#define RUN_TEST_KNOWN_BUG_WITH(fn, issue, flags) run_known_bug_case(#fn, fn, issue, flags)

// Only ASan/UBSan can see the bug (a silent overflow, a use-after-free that
// may or may not show, undefined behaviour). The test runs only in sanitizer
// builds and is a SKIP elsewhere, where it can pass with the bug present.
#define KNOWN_BUG_SANITIZER_ONLY 0x1
// Skip the vmemory leak check, as RUN_TEST_IGNORE_LEAKS does.
#define KNOWN_BUG_IGNORE_LEAKS 0x2

void run_known_bug_case(const char* name, void (*fn)(void), u64 issue, u32 flags);

/*
 * Prints the XPASS, FAIL, XFAIL and SKIP results of every test case run so
 * far, one line each. Called by test_entry.c after the last suite.
 */
void print_test_summary(void);

/*
 * Writes size bytes of data to path, replacing any existing file.
 * Returns FALSE if the file could not be written.
 */
b8 write_test_file(const char* path, const void* data, u64 size);

/* write_test_file for a zero-terminated string (the terminator is not written). */
b8 write_test_text_file(const char* path, const char* text);

/* Deletes a file made by write_test_file. Missing files are ignored. */
void remove_test_file(const char* path);
