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
 */

extern u64 g_test_failures;
extern u64 g_test_checks;

#define CHECK(expr, msg) do {                                                   \
    g_test_checks++;                                                            \
    if(!(expr)){                                                                \
      g_test_failures++;                                                        \
      VERROR("FAIL %s:%d: %s (%s)", __FILE__, __LINE__, msg, #expr);            \
    }                                                                           \
  } while(0)

/*
 * Stops the current test case when a CHECK precondition would make the rest
 * of it unsafe (e.g. a NULL pointer that later lines dereference).
 */
#define REQUIRE(expr, msg) do {                                                 \
    g_test_checks++;                                                            \
    if(!(expr)){                                                                \
      g_test_failures++;                                                        \
      VERROR("FAIL %s:%d: %s (%s)", __FILE__, __LINE__, msg, #expr);            \
      return;                                                                   \
    }                                                                           \
  } while(0)

/*
 * Runs one test case and checks it returned every byte it took from vmemory.
 */
#define RUN_TEST(fn) do {                                                       \
    u64 _before = get_memory_allocated();                                       \
    VINFO("  %s", #fn);                                                         \
    fn();                                                                       \
    u64 _after = get_memory_allocated();                                        \
    CHECK(_after == _before, #fn " leaked memory tracked by vmemory");          \
  } while(0)
