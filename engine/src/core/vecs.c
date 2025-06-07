#include "vecs.h"
#include "core/logger.h"
#include "core/vmemory.h"

b8 initilize_entity_component_system(entity_component_system *componentLists){
  componentLists->list = darray_new(sizeof(vcomponent_list));
  return TRUE;
}

u64 get_new_entity_id(){
  static u64 newID = 0;
  u64 retVal = newID;
  newID++;
  return retVal;
}

b8 get_component_list(entity_component_system componentLists, vcomponent_list *outList, vcomponent_type requestedType){
  vcomponent_list *compLists = componentLists.list->data;
  for(int i = 0; i < componentLists.list->length; i++){
    if(compLists[i].type == requestedType){
      vcopy_memory(outList, &compLists[i], sizeof(vcomponent_list));
      return TRUE;
    }
  }
  return FALSE;
}

b8 register_component(entity_component_system componentLists, const vcomponent *comp, vcomponent_type compType){
  vcomponent_list compTypeList;
  if(get_component_list(componentLists, &compTypeList, compType) == FALSE){
    vcomponent_list newList = {
      .type = compType,
      .data = darray_new(sizeof(vcomponent)),
    };
    darray_push(newList.data, comp);
    darray_push(componentLists.list, &newList);
    return TRUE;
  }
  vcomponent *comps = (vcomponent*)compTypeList.data->data;
  for(int i = 0; i < compTypeList.data->length; i++){
    if(comps[i].entityID == comp->entityID){
      return FALSE;
    }
    if(comps[i].entityID > comp->entityID){
      darray_insert(compTypeList.data, comp, i);
      return TRUE;
    }
  }
  // If we got here, that means comp has the highest entityID in the list, just push it to the end
  darray_push(compTypeList.data, comp);
  return TRUE;
}

b8 delete_component(entity_component_system componentLists, vcomponent_type type, u64 entityID){
  vcomponent_list compTypeList;
  if(get_component_list(componentLists, &compTypeList, type) == FALSE){
    return FALSE;
  }
  vcomponent *comps = compTypeList.data->data;
  for(int i = 0; i < compTypeList.data->length; i++){
    if(comps[i].entityID == entityID){
      darray_delete(compTypeList.data->data, i);
      return TRUE;
    }
  }
  return FALSE;
}