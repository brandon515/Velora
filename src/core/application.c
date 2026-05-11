#include "application.h"
#include "utils/vgltf.h"
#include "utils/vstring.h"
#include "core/vecs.h"
#include "components/vcamera.h"
#include "components/vtransform.h"

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

#include "utils/vjson.h"
b8 application_run(application_state* app_state){
  /*gltf_object obj = {0};
  if(import_gltf("Models/CameraTest/scene.gltf", &obj) == TRUE){
  free_gltf(&obj);
  }*/

  u64 mainCamera = create_new_entity();
  if(register_empty_transform(mainCamera) == FALSE){
    VERROR("Unable to register transform");
    return FALSE;
  }
  if(register_camera(mainCamera, TRUE) == FALSE){
    VERROR("Unable to register Camera");
    return FALSE;
  }
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