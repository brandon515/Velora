#include "input.h"
#include "utils/vstring.h"
#include "core/event.h"
#include "core/vmemory.h"

b8 action_handler(event* event, void* state){
  input_action* action = (input_action*)state;
  if(event->event_type == ENGINE_INPUT_BUTTON){
    //
  }else if(event->event_type == ENGINE_MOUSE_BUTTON){
    //
  }else if(event->event_type == ENGINE_MOUSE_POSITION){
    //
  }else if(event->event_type == ENGINE_MOUSE_WHEEL){
    //
  }
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
  state->input_mappings = darray_new(sizeof(input_mapping*));
  state->input_actions = darray_new(sizeof(input_action*));
  return TRUE;
}

b8 register_input_mapping(input_state* state, const char* input_name, u32* out_id){
  static u64 new_id = 0;
  input_mapping** curMaps = (input_mapping**)state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(vstrcmp(curMaps[i]->name, input_name) == TRUE){
      return FALSE;
    }
  }
  input_mapping new_mapping = {
    .id = new_id,
    .name = input_name,
    .curValue = 0,
  };
  darray_push(state->input_mappings, &new_mapping);
  new_id++;
  return TRUE;
}

input_mapping* get_input_mapping(input_state* state, u64 mapping_id){
  input_mapping** curMaps = state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(curMaps[i]->id == mapping_id){
      return curMaps[i];
    }
  }
  return NULL;
}

input_action* get_input_action(input_state* state, u64 action_id){
  input_action** curActions = state->input_actions->data;
  for(int i = 0; i < state->input_actions->length; i++){
    if(curActions[i]->action_id == action_id){
      return curActions[i];
    }
  }
  return NULL;
}

b8 bind_input_action(input_state* state, u64 mapping_id, i64 action_id){
  input_mapping* mapping = get_input_action_darray(state, mapping_id);
  if(mapping == NULL){
    return FALSE;
  }
  input_action* action = get_input_action(state, action_id);
  if(action == NULL){
    input_action* newAction = vallocate(sizeof(input_action), MEMORY_TAG_INPUT_DATA);
    newAction->action_id = action_id;
    newAction->listener_count = 0;
    newAction->mapping = darray_new(sizeof(input_mapping*));
    register_input_action_liseners(newAction);
    darray_push(state->input_actions, &newAction);
  }
  darray_push(action->mapping, mapping);
  return TRUE;
}

b8 unbind_input_action(input_state* state, u64 mapping_id, i64 action_id){
  input_action* action = get_input_action(state, action_id);
  input_mapping** mappings = (input_mapping**) action->mapping->data;
  for(int i = 0; i < action->mapping->length; i++){
    if(mappings[i]->id == mapping_id){
      darray_delete(action->mapping, i);
      return TRUE;
    }
  }
  return FALSE;
}

b8 shutdown_input_sytem(input_state *state){
  darray_free(state->input_mappings);
  darray_free(state->input_actions);
  return TRUE;
}
