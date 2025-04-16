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
    .value = 0,
    .actions = darray_new(sizeof(input_action*)),
  };
  darray_push(state->input_mappings, &new_mapping);
  new_id++;
  return TRUE;
}

b8 bind_input_action(input_state* state, u32 mapping_id, u64 action_id, u8 action_value, b8 is_positive_mapping){
  input_mapping* curMaps = state->input_mappings->data;
  darray* mapping = NULL;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(curMaps[i].id == mapping_id){
      mapping = curMaps[i].actions;
      break;
    }
  }
  if(mapping == NULL){
    return FALSE;
  }
  input_action* curActions = (input_action*)mapping->data;
  for(int i = 0; i < mapping->length; i++){
    if(curActions[i].action_id == action_id){
      curActions[i].valueToMap = action_value;
      return TRUE;
    }
  }
  input_action* newAction = vallocate(sizeof(input_action), MEMORY_TAG_INPUT_DATA);
  newAction->action_id = action_id;
  newAction->valueToMap = action_value;
  newAction->listener_count = 0;
  darray_push(mapping, &newAction);
  return TRUE;
}

b8 shutdown_input_sytem(input_state *state){
  darray_free(state->input_mappings);
  return TRUE;
}
