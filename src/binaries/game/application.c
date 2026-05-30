#include "application.h"
#include "core/logger.h"
#include "defines.h"
#include "render/velora_render.h"
#include "render/vmesh.h"
#include "core/utils/vgltf.h"
#include "core/vmemory.h"
#include "ecs/vecs.h"
#include "ecs/vcamera.h"
#include "ecs/vtransform.h"
#include "ecs/vrenderable.h"

const char *NAME = "Velora";
const i16 START_POS_X = 0;
const i16 START_POS_y = 0;
const i16 START_WIDTH = 800;
const i16 START_HEIGHT = 600;

b8 close_event_handler(event* event, void* state){
  application_state* app_state = (application_state*)state;
  app_state->is_running = FALSE;
  return FALSE;
}

b8 application_start(application_state *app_state){
  app_state->render_state = vallocate(sizeof(render_state), MEMORY_TAG_RENDERER);
  app_state->is_running = TRUE;
  app_state->is_suspended = FALSE;
  app_state->width = START_WIDTH;
  app_state->height = START_HEIGHT;

  if(platform_startup(
  &app_state->platform, 
  NAME,
  START_POS_X,
  START_POS_y,
  app_state->width,
  app_state->height) == FALSE){
    return FALSE;
  }

  if(initiate_render_system(app_state->render_state, NAME, &app_state->platform) == FALSE){
    platform_console_write("Unable to initiate render system", LOG_LEVEL_FATAL);
    return FALSE;
  }
  initiate_input_system(&app_state->input);
  initilize_entity_component_system();

  register_listener(ENGINE_CLOSE_GAME, close_event_handler, app_state);
  register_listener(ENGINE_WINDOW_RESIZE, resize_handler, app_state->render_state);// defined in the renderer code
  
  return TRUE;
}

b8 application_run(application_state* app_state){
  gltf_object obj = {0};
  VEL_CHECK(import_gltf("Cube.gltf", &obj))
  vmesh cube;
  VEL_CHECK(vmesh_from_gltf(&obj, 0, &cube));
  u64 renderableMeshHandle;
  VEL_CHECK(register_mesh(app_state->render_state, cube, &renderableMeshHandle));
  free_gltf(&obj);

  u64 mainCamera = create_new_entity();
  VEL_CHECK_MSG(register_empty_transform(mainCamera, NULL), "Unable to register transform for main camera");
  VEL_CHECK_MSG(register_camera(mainCamera, TRUE), "Unable to register Camera");
  u64 firstRenderable = create_new_entity();
  VEL_CHECK_MSG(register_transform(firstRenderable, 0, 0, -10, 45, 45, 0, 1, 1, 1, NULL), "Unable to register transform for first renderable");
  VEL_CHECK_MSG(register_renderable(firstRenderable, renderableMeshHandle, 0), "Unable to register renderable");
  while(app_state->is_running){
    platform_pump_messages(&app_state->platform);
    pump_events(99.9f);
    if(app_state->is_suspended == FALSE){
      render_preframe(app_state->render_state);
      render_frame(app_state->render_state);
      render_postframe(app_state->render_state);
    }
  }
  platform_shutdown(&app_state->platform);
  shutdown_render_system(app_state->render_state);
  shutdown_input_sytem(&app_state->input);
  shutdown_entity_component_system();

  return TRUE;
}