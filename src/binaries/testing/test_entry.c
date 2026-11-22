#include "test_framework.h"
#include "darray_test.h"
#include "vmemory_test.h"
#include "core/memory/vmemory.h"
#include "core/logger.h"

#include <string.h>

u64 g_test_failures = 0;
u64 g_test_checks = 0;

typedef struct _test_suite {
  const char* name;
  void (*run)(void);
} test_suite;

static const test_suite suites[] = {
  {"vmemory", vmemory_test},
  {"darray", darray_test},
};

#define SUITE_COUNT (sizeof(suites) / sizeof(suites[0]))

/*
 * Usage: velora_tests [suite]
 * With no argument every suite runs. Exit code is 0 only if every CHECK passed.
 */
int main(int argc, char** argv) {
  initialize_memory();
  initialize_logging();

  const char* filter = argc > 1 ? argv[1] : NULL;
  b8 matched = FALSE;
  for(u64 i = 0; i < SUITE_COUNT; i++) {
    if(filter != NULL && strcmp(filter, suites[i].name) != 0) {
      continue;
    }
    matched = TRUE;
    VINFO("Suite: %s", suites[i].name);
    suites[i].run();
  }

  if(!matched) {
    VERROR("No test suite named '%s'", filter);
    g_test_failures++;
  }

  if(g_test_failures == 0) {
    VINFO("All %llu checks passed", g_test_checks);
  } else {
    VERROR("%llu of %llu checks failed", g_test_failures, g_test_checks);
  }

  shutdown_logging();
  shutdown_memory();
  return g_test_failures == 0 ? 0 : 1;
}
