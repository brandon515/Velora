#include "ecs/vcamera.h"
#include "ecs/vecs.h"

b8 register_camera(u64 entityID, b8 active){
  vtransform *trans = get_transform_component(entityID);
  if(trans == NULL){
    return FALSE;
  }
  vcamera newCam = {
    .active = active,
    .transform = trans,
  };
  return attach_component(VELORA_COMPONENT_CAMERA, entityID, sizeof(newCam), &newCam);
}

vcamera* camera_get_active(){
  darray *cameraComps;
  if(get_components(VELORA_COMPONENT_CAMERA, &cameraComps) == FALSE){
    return NULL;
  }
  iterator cameraIt = darray_create_iterator(cameraComps);
  vcomponent *activeCamera;
  while(iterator_next(&cameraIt, (void**)&activeCamera)){
    vcamera *camData = activeCamera->data;
    if(camData->active == TRUE){
      return camData;
    }
  }
  return NULL;
}