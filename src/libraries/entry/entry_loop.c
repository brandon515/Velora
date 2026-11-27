#include "entry_loop.h"
#include "core/event.h"
#include "core/logger.h"

static b8 close_event_handler(event* event, void* state) {
  velora_loop_state* loop = (velora_loop_state*)state;
  loop->is_running = FALSE;
  return FALSE;
}

b8 velora_run_application(velora_loop_state* loop, const velora_app_callbacks* app, const velora_frame_hooks* hooks, int argc, char** argv) {
  loop->is_running = TRUE;
  loop->is_suspended = FALSE;
  loop->app_data = NULL;

  if(app->init(&loop->app_data, argc, argv) == FALSE) {
    VFATAL("Application unable to start");
    return FALSE;
  }

  u64 close_listener = register_listener(ENGINE_CLOSE_GAME, close_event_handler, loop);
  while(loop->is_running) {
    loop->is_running = app->frame(loop->app_data);
    if(hooks->pump_messages != NULL) {
      hooks->pump_messages(hooks->context);
    }
    pump_events(99.9f);
    if(loop->is_suspended == FALSE && hooks->render != NULL) {
      hooks->render(hooks->context);
    }
  }
  deregister_listener(close_listener);

  app->exit(loop->app_data);
  return TRUE;
}
