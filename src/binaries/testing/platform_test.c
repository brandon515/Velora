#include "platform_test.h"
#include "test_framework.h"
#include "core/platform/platform.h"

#include <string.h>
#include <time.h>

static f64 wall_seconds(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
}

static void test_memory_helpers(void) {
  u8 buf[16];
  platform_set_memory(buf, 0xAB, sizeof(buf));
  CHECK(buf[0] == 0xAB && buf[15] == 0xAB, "platform_set_memory");
  platform_zero_memory(buf, sizeof(buf));
  CHECK(buf[0] == 0 && buf[15] == 0, "platform_zero_memory");
  u8 src[4] = {1, 2, 3, 4};
  platform_copy_memory(buf, src, sizeof(src));
  CHECK(memcmp(buf, src, sizeof(src)) == 0, "platform_copy_memory");
  // Overlapping move to the right.
  platform_move_memory(buf + 1, buf, 4);
  CHECK(buf[1] == 1 && buf[2] == 2 && buf[3] == 3 && buf[4] == 4, "platform_move_memory handles overlap");
}

static void test_allocate_and_reallocate(void) {
  u8* p = platform_allocate(64, FALSE);
  REQUIRE(p != NULL, "platform_allocate");
  p[63] = 9;
  p = platform_reallocate(p, 4096);
  REQUIRE(p != NULL, "platform_reallocate");
  CHECK(p[63] == 9, "reallocate keeps contents");
  platform_free(p, FALSE);
}

// Tier 3: platform_sleep is empty on Linux, so the main loop busy-spins while minimised.
static void test_sleep_waits(void) {
  f64 start = wall_seconds();
  platform_sleep(50);
  f64 elapsed = wall_seconds() - start;
  CHECK(elapsed >= 0.040, "platform_sleep(50) waits at least ~50 ms");
}

#if VPLATFORM_LINUX
/*
 * bug-report Tier 2 note: platform_get_absolute_time returns 0 on Linux,
 * which hides the pump_events bug and breaks any timing. (On Windows the
 * clock is only initialised by platform_startup, which needs a window, so
 * this runs on Linux only.)
 */
static void test_absolute_time_advances(void) {
  f64 t0 = platform_get_absolute_time();
  f64 start = wall_seconds();
  while(wall_seconds() - start < 0.02) {
  }
  f64 t1 = platform_get_absolute_time();
  CHECK(t1 > t0, "platform_get_absolute_time advances");
  CHECK(t1 - t0 >= 0.015 && t1 - t0 < 1.0, "platform_get_absolute_time is in seconds");
}
#endif

void platform_test(void) {
  RUN_TEST(test_memory_helpers);
  RUN_TEST(test_allocate_and_reallocate);
  RUN_TEST(test_sleep_waits);
#if VPLATFORM_LINUX
  RUN_TEST(test_absolute_time_advances);
#endif
}
