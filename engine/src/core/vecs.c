#include "vecs.h"
#include "core/logger.h"
#include "core/vmemory.h"

static entity_component_system componentLists;
static b8 ECS_INIT = FALSE;

b8 initilize_entity_component_system(){
  if(ECS_INIT == TRUE){
    return FALSE;
  }
  componentLists.list = darray_new(sizeof(vcomponent_list));
  ECS_INIT = TRUE;
  return TRUE;
}

u64 get_new_entity_id(){
  static u64 newID = 0;
  return newID++;
}

b8 get_component_list(vcomponent_list *outList, vcomponent_type requestedType){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  vcomponent_list *compLists = componentLists.list->data;
  for(int i = 0; i < componentLists.list->length; i++){
    if(compLists[i].type == requestedType){
      vcopy_memory(outList, &compLists[i], sizeof(vcomponent_list));
      return TRUE;
    }
  }
  return FALSE;
}

b8 register_component(const vcomponent *comp, vcomponent_type compType){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  vcomponent_list compTypeList;
  if(get_component_list(&compTypeList, compType) == FALSE){
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

void free_component(vcomponent comp){
  vfree(comp.data, comp.dataSize, MEMORY_TAG_ECS);
}

void free_component_list(vcomponent_list compList){
  vcomponent *comps = compList.data->data;
  for(int i = 0; i < compList.data->length; i++){
    free_component(comps[i]);
  }
  darray_free(compList.data);
}

b8 free_entity_component_system(){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  vcomponent_list *compLists = componentLists.list->data;
  for(int i = 0; i < componentLists.list->length; i++){
    free_component_list(compLists[i]);
  }
  darray_free(componentLists.list);
  ECS_INIT = FALSE;
  return TRUE;
}

b8 delete_component(vcomponent_type type, u64 entityID){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  vcomponent_list compTypeList;
  if(get_component_list(&compTypeList, type) == FALSE){
    return FALSE;
  }
  vcomponent *comps = compTypeList.data->data;
  for(int i = 0; i < compTypeList.data->length; i++){
    if(comps[i].entityID == entityID){
      free_component(comps[i]);
      darray_delete(compTypeList.data->data, i);
      return TRUE;
    }
  }
  return FALSE;
}

b8 get_components(vcomponent_type type, vcomponent **outList, u64 *outLength){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  vcomponent_list *data = componentLists.list->data;
  for(int i = 0; i < componentLists.list->length; i++){
    if(data[i].type == type){
      (*outLength) = data[i].data->length;
      (*outList) = data[i].data->data;//vallocate((*outLength)*sizeof(vcomponent), MEMORY_TAG_ECS);
      //vcopy_memory((*outList), data[i].data->data, sizeof(vcomponent)*(*outLength));
      return TRUE;
    }
  }
  return FALSE;
}

b8 delete_entity(u64 entityID){
  if(ECS_INIT == FALSE){
    return FALSE;
  }
  b8 entityExisted = FALSE;
  vcomponent_list *compTypeList = componentLists.list->data;
  for(int i = 0; i < componentLists.list->length; i++){
    vcomponent *components = compTypeList[i].data->data;
    for(int j = 0; j < compTypeList[i].data->length; j++){
      if(components[i].entityID == entityID){
        free_component(components[i]);
        darray_delete(compTypeList[i].data, i);
        entityExisted = TRUE;
        break;
      }
    }
  }
  return entityExisted;
}