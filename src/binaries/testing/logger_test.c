#include "logger_test.h"
#include "test_framework.h"
#include "core/logger.h"

static void test_short_messages(void) {
  // Only checks these return; the output goes to the test log.
  VINFO("    logger test: %s %d %llu %f", "string", -3, 42ULL, 1.5);
  VWARN("    logger test warning");
  VERROR("    logger test error (expected, not a failure)");
  CHECK(TRUE, "logging short messages returns");
}

/*
 * Tier 2 (logger.c:36): log_output formats the message into a 32000-byte
 * buffer and then sprintf's the level prefix plus that message into another
 * 32000-byte buffer, so a message near the limit overflows the stack.
 */
static void test_message_at_buffer_limit(void) {
  // vsnprintf keeps 31999 characters, and the 9-character level prefix
  // pushes the second sprintf past 32000.
  enum { LEN = 31999 };
  char* message = vallocate(LEN + 1, MEMORY_TAG_APPLICATION);
  vset_memory(message, 'x', LEN);
  message[LEN] = 0;
  VINFO("%s", message);
  vfree(message, LEN + 1, MEMORY_TAG_APPLICATION);
  CHECK(TRUE, "logging a message near the 32000 character limit returns without overflowing");
}

void logger_test(void) {
  RUN_TEST(test_short_messages);
  RUN_TEST_KNOWN_BUG_WITH(test_message_at_buffer_limit, 44, KNOWN_BUG_SANITIZER_ONLY);
}
