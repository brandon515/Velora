#define VELORA_MAIN_CALLBACKS
#ifdef VELORA_MAIN_CALLBACKS
#include "defines.h"
#include "core/entry/entry.h"
#include "core/logger.h"
#include "core/memory/vmemory.h"
#include "core/event.h"
#include "render/velora_render.h"
#include "ecs/vecs.h"

typedef struct _velora_state{
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  render_state render_state;
  void *app_data;
  i16 width;
  i16 height;
  f64 last_time;
} velora_state;

const char *NAME = "Velora";
const i16 START_POS_X = 0;
const i16 START_POS_y = 0;
const i16 START_WIDTH = 800;
const i16 START_HEIGHT = 600;

b8 close_event_handler(event* event, void* state){
  velora_state* app_state = (velora_state*)state;
  app_state->is_running = FALSE;
  return FALSE;
}

int main(int argc, char* argv[]){
  initialize_memory();
  initialize_logging();
  initiate_event_system();
  velora_state *engine_state = vallocate(sizeof(velora_state), MEMORY_TAG_APPLICATION);
  engine_state->is_running = TRUE;
  engine_state->is_suspended = FALSE;
  engine_state->width = START_WIDTH;
  engine_state->height = START_HEIGHT;
  engine_state->app_data = NULL;
  if(platform_startup(
    &engine_state->platform, 
    NAME,
    START_POS_X,
    START_POS_y,
    engine_state->width,
    engine_state->height) == FALSE)
  {
    return -2;
  }

  if(initiate_render_system(&engine_state->render_state, NAME, &engine_state->platform) == FALSE){
    VFATAL("Unable to initiate render system");
    return -4;
  }

  initilize_entity_component_system();

  register_listener(ENGINE_CLOSE_GAME, close_event_handler, engine_state);
  register_listener(ENGINE_WINDOW_RESIZE, resize_handler, &engine_state->render_state);// defined in the renderer code

  if(velora_init(&engine_state->app_data, argc, argv) == FALSE){
    VFATAL("Application unable to start");
    return -3;
  }

  while(velora_frame(engine_state->app_data)){
    platform_pump_messages(&engine_state->platform);
    pump_events(99.9f);
    if(engine_state->is_suspended == FALSE){
      render_preframe(&engine_state->render_state);
      render_frame(&engine_state->render_state);
      render_postframe(&engine_state->render_state);
    }
  }

  velora_exit(engine_state);
  // Shut down the renderer before the platform: the Vulkan surface and
  // swapchain reference the X11 display/window, so destroying them after
  // XCloseDisplay would dereference freed memory and segfault.
  shutdown_render_system(&engine_state->render_state);
  shutdown_entity_component_system();
  platform_shutdown(&engine_state->platform);
  shutdown_event_system();
  vfree(engine_state, sizeof(velora_state), MEMORY_TAG_APPLICATION);
  shutdown_logging();
  shutdown_memory();
  return 0;
}

b8 application_run(application_state* app_state){
  gltf_object obj = {0};
  VEL_CHECK(import_gltf("Cube.gltf", &obj))
  vmesh cube;
  VEL_CHECK(vmesh_from_gltf(&obj, 0, &cube));
  u64 renderableMeshHandle = 0;
  VEL_CHECK(register_mesh(app_state->render_state, cube, renderableMeshHandle));
  free_gltf(&obj);

  u64 mainCamera = create_new_entity();
  VEL_CHECK_MSG(register_empty_transform(mainCamera, NULL), "Unable to register transform for main camera");
  VEL_CHECK_MSG(register_camera(mainCamera, TRUE), "Unable to register Camera");
  u64 firstRenderable = create_new_entity();
  VEL_CHECK_MSG(register_transform(firstRenderable, 0, 0, -10, 45, 45, 0, 1, 1, 1, NULL), "Unable to register transform for first renderable");
  VEL_CHECK_MSG(register_renderable(firstRenderable, renderableMeshHandle, 0), "Unable to register renderable");
  vtransform *renTrans = get_transform_component(firstRenderable);
  while(app_state->is_running){
    platform_pump_messages(&app_state->platform);
    pump_events(99.9f);
    if(app_state->is_suspended == FALSE){
      renTrans->rotation.y += .5;
      render_preframe(app_state->render_state);
      render_frame(app_state->render_state);
      render_postframe(app_state->render_state);
    }
  }

  return TRUE;
}
#endif