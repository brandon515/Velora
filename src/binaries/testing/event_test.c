#include "event_test.h"
#include "test_framework.h"
#include "core/event.h"

/*
 * The event system is global, so event_test() initialises it once and every
 * test deregisters its listeners and leaves the queue empty when it is done.
 * Event types start at ENGINE_EVENT_ID_END like a game's custom events would.
 */

enum {
  TEST_EVENT_A = ENGINE_EVENT_ID_END,
  TEST_EVENT_B,
  TEST_EVENT_C,
};

#define MAX_RECORDED 16

// How many events/listeners the re-entrancy tests add from inside a handler.
// Bigger than the capacity warm_up_event_system leaves, so the darrays grow.
#define HANDLER_BURST 100

typedef struct {
  u64 calls;
  u64 types[MAX_RECORDED];
  u64 payloads[MAX_RECORDED]; // first u64 of event_data, or U64_MAX if none
  b8 consume;
} recorder;

static b8 record_listener(event* e, void* state) {
  recorder* r = state;
  if(r->calls < MAX_RECORDED) {
    r->types[r->calls] = e->event_type;
    r->payloads[r->calls] = (e->event_data != NULL && e->event_data_size >= sizeof(u64)) ? *(u64*)e->event_data : U64_MAX;
  }
  r->calls++;
  return r->consume;
}

static void test_fire_reaches_listener_with_data(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  CHECK(id != U64_MAX, "register_listener returns an id");
  u64 payload = 1234;
  CHECK(create_and_fire_event(TEST_EVENT_A, sizeof(payload), &payload) == TRUE, "fire with a listener returns TRUE");
  CHECK(r.calls == 1, "listener called once");
  CHECK(r.types[0] == TEST_EVENT_A, "listener got the right event type");
  CHECK(r.payloads[0] == 1234, "listener got a copy of the event data");
  CHECK(deregister_listener(id) == TRUE, "deregister succeeds");
}

static void test_fire_copies_data(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  u64 payload = 5;
  create_and_queue_event(TEST_EVENT_A, sizeof(payload), &payload);
  payload = 6; // the queued event must hold its own copy
  pump_events(1000.0);
  CHECK(r.calls == 1 && r.payloads[0] == 5, "queued event carries the value at queue time");
  deregister_listener(id);
}

static void test_listener_only_gets_its_type(void) {
  recorder ra = {0};
  recorder rb = {0};
  u64 ida = register_listener(TEST_EVENT_A, record_listener, &ra);
  u64 idb = register_listener(TEST_EVENT_B, record_listener, &rb);
  create_and_fire_event(TEST_EVENT_B, 0, NULL);
  create_and_fire_event(TEST_EVENT_B, 0, NULL);
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(ra.calls == 1, "A listener only sees A events");
  CHECK(rb.calls == 2, "B listener only sees B events");
  deregister_listener(ida);
  deregister_listener(idb);
}

static void test_consuming_listener_stops_propagation(void) {
  recorder first = {.consume = TRUE};
  recorder second = {0};
  u64 id1 = register_listener(TEST_EVENT_A, record_listener, &first);
  u64 id2 = register_listener(TEST_EVENT_A, record_listener, &second);
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(first.calls == 1, "first listener sees the event");
  CHECK(second.calls == 0, "a consumed event does not reach later listeners");
  first.consume = FALSE;
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(first.calls == 2 && second.calls == 1, "a non-consumed event reaches every listener");
  deregister_listener(id1);
  deregister_listener(id2);
}

static void test_deregister(void) {
  recorder r1 = {0};
  recorder r2 = {0};
  u64 id1 = register_listener(TEST_EVENT_A, record_listener, &r1);
  u64 id2 = register_listener(TEST_EVENT_A, record_listener, &r2);
  CHECK(id1 != id2, "listener ids are unique");
  CHECK(deregister_listener(id1) == TRUE, "deregister a registered listener");
  CHECK(deregister_listener(id1) == FALSE, "deregister the same id twice returns FALSE");
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(r1.calls == 0, "deregistered listener is not called");
  CHECK(r2.calls == 1, "remaining listener still called");
  deregister_listener(id2);
}

static void test_fire_unlistened_type_frees_data(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  u64 payload = 7;
  create_and_fire_event(TEST_EVENT_C, sizeof(payload), &payload);
  CHECK(r.calls == 0, "no listener for type C was called");
  deregister_listener(id);
  // RUN_TEST's leak check verifies the event data was freed.
}

static void test_pump_delivers_each_event_once_in_order(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  for(u64 i = 0; i < 5; i++) {
    CHECK(create_and_queue_event(TEST_EVENT_A, sizeof(i), &i) == TRUE, "queue event");
  }
  CHECK(r.calls == 0, "queued events are not delivered before pumping");
  CHECK(pump_events(1000.0) == TRUE, "pump with plenty of time empties the queue");
  CHECK(r.calls == 5, "each queued event delivered once");
  b8 in_order = TRUE;
  for(u64 i = 0; i < 5; i++) {
    in_order = in_order && r.payloads[i] == i;
  }
  CHECK(in_order, "events delivered in queue order");
  CHECK(pump_events(1000.0) == TRUE, "pumping an empty queue succeeds");
  CHECK(r.calls == 5, "pumping again delivers nothing new");
  deregister_listener(id);
}

/*
 * Tier 2: fire_event with no listeners at all returns before freeing the
 * event data, so create_and_fire_event leaks it.
 */
static void test_fire_with_no_listeners_frees_data(void) {
  u64 payload = 9;
  CHECK(create_and_fire_event(TEST_EVENT_A, sizeof(payload), &payload) == FALSE, "fire with no listeners returns FALSE");
  // RUN_TEST's leak check verifies the event data was freed.
}

/*
 * Tier 2: when pump_events runs out of time after firing event i it drains
 * only i events, so event i stays queued and fires again on the next pump.
 * A negative time limit makes it stop after the first event even though
 * platform_get_absolute_time() currently returns 0.
 */
static void test_pump_over_time_limit_does_not_repeat_events(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  for(u64 i = 0; i < 3; i++) {
    create_and_queue_event(TEST_EVENT_A, 0, NULL);
  }
  CHECK(pump_events(-1.0) == FALSE, "pump that runs out of time returns FALSE");
  u64 after_first = r.calls;
  CHECK(after_first >= 1, "pump delivers at least one event before stopping");
  for(u64 i = 0; i < 5 && pump_events(-1.0) == FALSE; i++) {
  }
  pump_events(1000.0);
  CHECK(r.calls == 3, "three queued events are delivered exactly three times in total");
  deregister_listener(id);
}

// Same as above with event data, where the repeat also frees the data twice.
static void test_pump_over_time_limit_does_not_double_free(void) {
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  for(u64 i = 0; i < 3; i++) {
    create_and_queue_event(TEST_EVENT_A, sizeof(i), &i);
  }
  pump_events(-1.0);
  pump_events(1000.0);
  CHECK(r.calls == 3, "three queued events are delivered exactly three times in total");
  CHECK(r.payloads[0] == 0 && r.payloads[1] == 1 && r.payloads[2] == 2, "each event delivered with its own data, in order");
  deregister_listener(id);
}

// A listener that queues a follow-up event, as gameplay code would.
static b8 requeue_listener(event* e, void* state) {
  recorder* r = state;
  r->calls++;
  if(e->event_type == TEST_EVENT_A) {
    // More than the warmed-up capacity, so the queue's darray has to grow
    // while pump_events walks it.
    for(u64 i = 0; i < HANDLER_BURST; i++) {
      create_and_queue_event(TEST_EVENT_B, sizeof(i), &i);
    }
  }
  return FALSE;
}

/*
 * Tier 2: pump_events keeps a pointer into the queue's darray while handlers
 * run, so a handler that queues events can reallocate it out from under the
 * loop, and the final darray_clear throws the newly queued events away.
 */
static void test_event_queued_by_handler_is_delivered_later(void) {
  recorder ra = {0};
  recorder rb = {0};
  u64 ida = register_listener(TEST_EVENT_A, requeue_listener, &ra);
  u64 idb = register_listener(TEST_EVENT_B, record_listener, &rb);
  create_and_queue_event(TEST_EVENT_A, 0, NULL);
  pump_events(1000.0);
  pump_events(1000.0);
  CHECK(ra.calls == 1, "A handled once");
  CHECK(rb.calls == HANDLER_BURST, "every event queued by the A handler is delivered");
  deregister_listener(ida);
  deregister_listener(idb);
  pump_events(1000.0);
}

static u64 g_late_listener_id = U64_MAX;
static u64 g_extra_ids[HANDLER_BURST];
static recorder g_late_recorder;

static b8 registering_listener(event* e, void* state) {
  recorder* r = state;
  r->calls++;
  if(g_late_listener_id == U64_MAX) {
    // More than the warmed-up capacity, so the listener darray has to grow mid-dispatch.
    g_late_listener_id = register_listener(TEST_EVENT_B, record_listener, &g_late_recorder);
    for(u64 i = 0; i < HANDLER_BURST; i++) {
      g_extra_ids[i] = register_listener(TEST_EVENT_C, record_listener, &g_late_recorder);
    }
  }
  return FALSE;
}

/*
 * Tier 2: fire_event also keeps a pointer into the listener darray, so a
 * handler that registers a listener can reallocate it during dispatch.
 */
static void test_listener_registered_by_handler(void) {
  recorder r = {0};
  recorder after = {0};
  g_late_listener_id = U64_MAX;
  g_late_recorder = (recorder){0};
  u64 id = register_listener(TEST_EVENT_A, registering_listener, &r);
  u64 id_after = register_listener(TEST_EVENT_A, record_listener, &after);
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(r.calls == 1, "registering listener ran once");
  CHECK(after.calls == 1, "listener after it still runs");
  create_and_fire_event(TEST_EVENT_B, 0, NULL);
  CHECK(g_late_recorder.calls == 1, "listener registered during dispatch receives later events");
  deregister_listener(id);
  deregister_listener(id_after);
  deregister_listener(g_late_listener_id);
  for(u64 i = 0; i < HANDLER_BURST; i++) {
    deregister_listener(g_extra_ids[i]);
  }
}

/*
 * Issue #23: shutdown_event_system frees the queue and listener
 * arrays but leaves the pointers set, so initiate_event_system afterwards
 * keeps using freed memory. Kept last because on platforms without per-test
 * isolation it leaves the event system broken.
 */
static void test_restart_after_shutdown(void) {
  shutdown_event_system();
  initiate_event_system();
  recorder r = {0};
  u64 id = register_listener(TEST_EVENT_A, record_listener, &r);
  create_and_fire_event(TEST_EVENT_A, 0, NULL);
  CHECK(r.calls == 1, "event system works after shutdown and re-initialisation");
  deregister_listener(id);
}

/*
 * darrays never shrink, so the first tests to register listeners or queue
 * events would grow the system's arrays and look like leaks to RUN_TEST.
 * Grow them once here, outside any test.
 */
static void warm_up_event_system(void) {
  recorder r = {0};
  u64 ids[32];
  for(u64 i = 0; i < 32; i++) {
    ids[i] = register_listener(TEST_EVENT_C, record_listener, &r);
  }
  for(u64 i = 0; i < 32; i++) {
    create_and_queue_event(TEST_EVENT_C, 0, NULL);
  }
  pump_events(1000.0);
  for(u64 i = 0; i < 32; i++) {
    deregister_listener(ids[i]);
  }
}

void event_test(void) {
  initiate_event_system();
  warm_up_event_system();
  RUN_TEST(test_fire_reaches_listener_with_data);
  RUN_TEST(test_fire_copies_data);
  RUN_TEST(test_listener_only_gets_its_type);
  RUN_TEST(test_consuming_listener_stops_propagation);
  RUN_TEST(test_deregister);
  RUN_TEST(test_fire_unlistened_type_frees_data);
  RUN_TEST(test_pump_delivers_each_event_once_in_order);
  RUN_TEST(test_fire_with_no_listeners_frees_data);
  RUN_TEST(test_pump_over_time_limit_does_not_repeat_events);
  RUN_TEST(test_pump_over_time_limit_does_not_double_free);
  // These two grow the system's darrays on purpose (that growth is the bug
  // they look for), which RUN_TEST's leak check would count as a leak.
  RUN_TEST_IGNORE_LEAKS(test_event_queued_by_handler_is_delivered_later);
  RUN_TEST_IGNORE_LEAKS(test_listener_registered_by_handler);
  RUN_TEST_IGNORE_LEAKS(test_restart_after_shutdown);
  shutdown_event_system();
}
