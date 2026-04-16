#include "vcamera.h"
#include "core/vecs.h"

b8 register_camera(u64 entityID, b8 active){
  vcamera newCam = {
    .active = active,
  };
  return attach_component(VELORA_COMPONENT_CAMERA, entityID, sizeof(newCam), &newCam);
}