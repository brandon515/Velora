#include "input.h"
#include "utils/vstring.h"
#include "core/event.h"
#include "core/vmemory.h"

b8 action_handler(event* event, void* state){
  return FALSE;
}

void add_listener(input_action* action, event_listener func, event_id event){
  action->listener_id[action->listener_count] = register_listener(event, func, action);
  action->listener_count++;
}

void register_input_action_liseners(input_action* action){
  add_listener(action, action_handler, ENGINE_INPUT_BUTTON);
  add_listener(action, action_handler, ENGINE_MOUSE_BUTTON);
  add_listener(action, action_handler, ENGINE_MOUSE_POSITION);
  add_listener(action, action_handler, ENGINE_MOUSE_WHEEL);
}

void deregister_input_action_listeners(input_action* action){
  for(int i = 0; i < action->listener_count; i++){
    deregister_listener(action->listener_id[i]);
  }
}

b8 init_input_system(input_state* state){
  state->input_mappings = darray_new(sizeof(input_mapping));
  return TRUE;
}

b8 register_input_mapping(input_state* state, const char* input_name, u32* out_id){
  static u64 new_id = 0;
  input_mapping* curMaps = (input_mapping*)state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(vstrcmp(curMaps[i].name, input_name) == TRUE){
      return FALSE;
    }
  }
  input_mapping new_mapping = {
    .id = new_id,
    .name = input_name,
    .actions = darray_new(sizeof(input_action*)),
  };
  darray_push(state->input_mappings, &new_mapping);
  new_id++;
  return TRUE;
}

darray* get_input_action_darray(input_state* state, u64 mapping_id){
  input_mapping* curMaps = state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(curMaps[i].id == mapping_id){
      return curMaps[i].actions;
    }
  }
  return NULL;
}

b8 bind_input_action(input_state* state, u64 mapping_id, i64 action_id){
  darray* mapping = get_input_action_darray(state, mapping_id);
  if(mapping == NULL){
    return FALSE;
  }
  input_action* curActions = (input_action*)mapping->data;
  for(int i = 0; i < mapping->length; i++){
    if(curActions[i].action_id == action_id){
      return TRUE;
    }
  }
  input_action* newAction = vallocate(sizeof(input_action), MEMORY_TAG_INPUT_DATA);
  newAction->action_id = action_id;
  newAction->listener_count = 0;
  register_input_action_liseners(newAction);
  darray_push(mapping, &newAction);
  return TRUE;
}

b8 unbind_input_action(input_state* state, u64 mapping_id, i64 action_id){
  darray* mapping = get_input_action_darray(state, mapping_id);
  if(mapping == NULL){
    return FALSE;
  }
  input_action** actions = (input_action**)mapping->data;
  for(int i = 0; i < mapping->length; i++){
    if(actions[i]->action_id == action_id){
      input_action* curAction;
      darray_remove(mapping, i, &curAction);
      deregister_input_action_listeners(curAction);
      vfree(curAction, sizeof(input_action), MEMORY_TAG_INPUT_DATA);
      return TRUE;
    }
  }
  return FALSE;
}

b8 shutdown_input_sytem(input_state *state){
  darray_free(state->input_mappings);
  return TRUE;
}
