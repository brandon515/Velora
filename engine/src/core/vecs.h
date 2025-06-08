#pragma once
#include "defines.h"
#include "container/darray.h"

typedef struct _entity_component_system{
  darray *list;
}entity_component_system;

typedef enum _vcomponent_type{
  VELORA_COMPONENT_TRANSFORM,
}vcomponent_type;

typedef struct _vcomponent_list{
  vcomponent_type type;
  darray *data;
}vcomponent_list;

typedef struct _vcomponent{
  u64 entityID;
  void *data;
}vcomponent;

b8 initilize_entity_component_system();
u64 get_new_entity_id();
b8 register_component(const vcomponent *comp, vcomponent_type compType);
b8 delete_component(vcomponent_type type, u64 entityID);
b8 get_components(vcomponent_type type, vcomponent **outList, u64 *outLength);
b8 delete_entity(u64 entityID);
