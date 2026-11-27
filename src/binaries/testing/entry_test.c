#include "entry_test.h"
#include "test_framework.h"

#include "core/event.h"
#include "entry_loop.h"

/*
 * Drives the engine's main loop (src/libraries/entry/entry_loop.c) with a
 * fake application and fake per-frame hooks, so no window or GPU is needed.
 */

typedef struct _fake_app {
  int marker;          // init hands &marker back as the application's state
  u64 frames;          // frame calls so far
  u64 stop_after;      // frame returns FALSE on this call (0 = never)
  b8 init_result;
  void* frame_state;   // last state frame received
  void* exit_state;    // state exit received
  u64 exit_calls;
} fake_app;

typedef struct _fake_hooks {
  u64 pumps;
  u64 renders;
  u64 close_on_pump;   // queue ENGINE_CLOSE_GAME on this pump (0 = never)
} fake_hooks;

static fake_app app_under_test;

static b8 fake_init(void** state, int argc, char** argv) {
  *state = &app_under_test.marker;
  return app_under_test.init_result;
}

static b8 fake_frame(void* state) {
  app_under_test.frames++;
  app_under_test.frame_state = state;
  return app_under_test.stop_after == 0 || app_under_test.frames < app_under_test.stop_after;
}

static void fake_exit(void* state) {
  app_under_test.exit_calls++;
  app_under_test.exit_state = state;
}

static void fake_pump(void* context) {
  fake_hooks* hooks = (fake_hooks*)context;
  hooks->pumps++;
  if(hooks->pumps == hooks->close_on_pump) {
    create_and_queue_event(ENGINE_CLOSE_GAME, 0, NULL);
  }
}

static void fake_render(void* context) {
  ((fake_hooks*)context)->renders++;
}

static const velora_app_callbacks fake_callbacks = {fake_init, fake_frame, fake_exit};

static b8 run_loop(fake_hooks* counters) {
  velora_frame_hooks hooks = {counters, fake_pump, fake_render};
  velora_loop_state loop;
  initiate_event_system();
  b8 result = velora_run_application(&loop, &fake_callbacks, &hooks, 0, NULL);
  shutdown_event_system();
  return result;
}

static void reset_app(u64 stop_after) {
  app_under_test = (fake_app){0};
  app_under_test.stop_after = stop_after;
  app_under_test.init_result = TRUE;
}

// Regression test for issue #70: velora_exit was given the engine's state.
static void test_exit_receives_application_state(void) {
  reset_app(1);
  fake_hooks hooks = {0};
  CHECK(run_loop(&hooks) == TRUE, "the loop runs");
  CHECK(app_under_test.exit_calls == 1, "exit is called once");
  CHECK(app_under_test.exit_state == &app_under_test.marker, "exit gets the pointer init stored");
}

static void test_frame_receives_application_state(void) {
  reset_app(1);
  fake_hooks hooks = {0};
  run_loop(&hooks);
  CHECK(app_under_test.frame_state == &app_under_test.marker, "frame gets the pointer init stored");
}

static void test_frame_returning_false_stops_the_loop(void) {
  reset_app(3);
  fake_hooks hooks = {0};
  run_loop(&hooks);
  CHECK(app_under_test.frames == 3, "the loop stops on the frame that returned FALSE");
  CHECK(hooks.pumps == 3, "messages are pumped once per frame");
  CHECK(hooks.renders == 3, "the engine renders once per frame");
}

// Regression test for issue #22: closing the window didn't stop the loop.
static void test_close_event_stops_the_loop(void) {
  reset_app(0);
  fake_hooks hooks = {.close_on_pump = 2};
  run_loop(&hooks);
  CHECK(app_under_test.frames == 2, "the loop stops after the frame whose events held the close");
  CHECK(app_under_test.exit_calls == 1, "exit still runs after a close");
}

static void test_failed_init_skips_frame_and_exit(void) {
  reset_app(1);
  app_under_test.init_result = FALSE;
  fake_hooks hooks = {0};
  CHECK(run_loop(&hooks) == FALSE, "the loop reports the failed init");
  CHECK(app_under_test.frames == 0, "frame is never called");
  CHECK(app_under_test.exit_calls == 0, "exit is never called");
}

void entry_test(void) {
  RUN_TEST(test_exit_receives_application_state);
  RUN_TEST(test_frame_receives_application_state);
  RUN_TEST(test_frame_returning_false_stops_the_loop);
  RUN_TEST(test_close_event_stops_the_loop);
  RUN_TEST(test_failed_init_skips_frame_and_exit);
}
