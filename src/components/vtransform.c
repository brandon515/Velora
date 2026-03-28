#include "vtransform.h"
#include "core/vecs.h"

vtransform* get_transform_component(u64 entityID){
  return get_component_data(entityID, VELORA_COMPONENT_TRANSFORM);
}

b8 register_empty_transform(u64 entityID){
  return register_transform(entityID,
                            0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f,
                            1.0f, 1.0f, 1.0f);
}

b8 register_transform(u64 entityID, 
                      f32 posX, f32 posY, f32 posZ,
                      f32 rotX, f32 rotY, f32 rotZ,
                      f32 scaX, f32 scaY, f32 scaZ){
  vtransform newTrans = {
    .position = {{posX, posY, posZ}},
    .rotation = {{rotX, rotY, rotZ}},
    .scale = {{scaX, scaY, scaZ}}
  };
  return attach_component(VELORA_COMPONENT_TRANSFORM, entityID, sizeof(newTrans), &newTrans);
}

void transform_set_position(vtransform *transformComp, f32 posX, f32 posY, f32 posZ){
  transformComp->position.x = posX;
  transformComp->position.y = posY;
  transformComp->position.z = posZ;
}

void transform_set_rotation(vtransform *transformComp, f32 rotX, f32 rotY, f32 rotZ){
  transformComp->rotation.x = rotX;
  transformComp->rotation.y = rotY;
  transformComp->rotation.z = rotZ;
}

void transform_set_scale(vtransform *transformComp, f32 scaX, f32 scaY, f32 scaZ){
  transformComp->scale.x = scaX;
  transformComp->scale.y = scaY;
  transformComp->scale.z = scaZ;
}

void transform_move(vtransform *transformComp, f32 posX, f32 posY, f32 posZ){
  transformComp->position.x += posX;
  transformComp->position.y += posY;
  transformComp->position.z += posZ;
}

void transform_rotate(vtransform *transformComp, vec3 axis, f32 rotationDegrees){
  vec3 normalizedAxis = vec3_normalize(axis);
  vec3 rotationAdd = vec3_multiply_scalar(normalizedAxis, rotationDegrees);
  transformComp->rotation = vec3_add(transformComp->rotation, rotationAdd);
}

mat4 transform_get_model_matrix(vtransform *transformComp){
  return model_matrix(transformComp->position, euler_to_quat(transformComp->rotation), transformComp->scale);
}