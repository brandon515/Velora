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

b8 initilize_entity_component_system(entity_component_system *componentLists);