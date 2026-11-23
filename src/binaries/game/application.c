#include "core/event.h"
#include "core/logger.h"
#include "defines.h"
#include "render/velora_render.h"
#include "render/vmesh.h"
#include "assets/vgltf.h"
#include "ecs/vecs.h"
#include "ecs/vcamera.h"
#include "ecs/vtransform.h"
#include "ecs/vrenderable.h"

#define VELORA_MAIN_CALLBACKS

b8 velora_init(void **state, int argc, char **argv){
  gltf_object obj = {0};
  VEL_CHECK(import_gltf("Cube.gltf", &obj));
  vmesh cube;
  VEL_CHECK(vmesh_from_gltf(&obj, 0, &cube));
  u64 renderableMeshHandle = 0;
  register_mesh_data mesh_data = {
    .mesh = cube,
    .handle = 0,
  };
  VEL_CHECK(create_and_fire_event(ENGINE_REGISTER_MESH, sizeof(mesh_data), &mesh_data));
  free_gltf(&obj);

  u64 mainCamera = create_new_entity();
  VEL_CHECK_MSG(register_empty_transform(mainCamera, NULL), "Unable to register transform for main camera");
  VEL_CHECK_MSG(register_camera(mainCamera, TRUE), "Unable to register Camera");
  u64 firstRenderable = create_new_entity();
  VEL_CHECK_MSG(register_transform(firstRenderable, 0, 0, -10, 45, 45, 0, 1, 1, 1, NULL), "Unable to register transform for first renderable");
  VEL_CHECK_MSG(register_renderable(firstRenderable, renderableMeshHandle, 0), "Unable to register renderable");
  return TRUE;
}

b8 velora_frame(void *state){
  return TRUE;
}

void velora_exit(void *state){
}

