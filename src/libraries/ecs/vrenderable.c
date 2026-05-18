#include "ecs/vrenderable.h"
#include "ecs/vecs.h"

b8 register_renderable(u64 entityID, u64 meshHandle, u64 materialHandle){
  vtransform *trans = get_transform_component(entityID);
  if(trans == NULL){
    return FALSE;
  }
  vrenderable comp = {
    .meshHandle = meshHandle,
    .materialHandle = materialHandle,
    .transform = trans,
  };
  return attach_component(VELORA_COMPONENT_RENDERABLE, entityID, sizeof(comp), &comp);
}