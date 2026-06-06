#include "core/input.h"
#include "core/utils/vstring.h"
#include "core/event.h"
#include "core/memory/vmemory.h"

b8 action_handler(event* curEvent, void* state){
  input_binding* binding = (input_binding*)state;
  event newEvent ={
    .event_type = ENGINE_GAME_INPUT,
    .event_data = binding->gInput,
    .event_data_size = 0 //we don't want the event system to free the game input
  };
  if(curEvent->event_type == ENGINE_INPUT_BUTTON){
    button_data* data = (button_data*)curEvent->event_data;
    if(data->button_code == binding->action_data){
      if(data->pressed == TRUE && binding->influencingInputValue == FALSE){
        binding->gInput->curValue += binding->valueToMap;
        binding->influencingInputValue = TRUE;
        fire_event(&newEvent);
      }else if(data->pressed == FALSE && binding->influencingInputValue == TRUE){
        binding->gInput->curValue -= binding->valueToMap;
        binding->influencingInputValue = FALSE;
        fire_event(&newEvent);
      }
    }
  }else if(curEvent->event_type == ENGINE_MOUSE_BUTTON){
    mouse_button_data* data = (mouse_button_data*)curEvent->event_data;
    if(data->button_code == binding->action_data){
      if(data->pressed == TRUE && binding->influencingInputValue == FALSE){
        binding->gInput->curValue += binding->valueToMap;
        binding->influencingInputValue = TRUE;
        fire_event(&newEvent);
      }else if(data->pressed == FALSE && binding->influencingInputValue == TRUE){
        binding->gInput->curValue -= binding->valueToMap;
        binding->influencingInputValue = FALSE;
        fire_event(&newEvent);
      }
    }
  }else if(curEvent->event_type == ENGINE_MOUSE_POSITION){
    mouse_position_data* data = (mouse_position_data*)curEvent->event_data;
    if(binding->action_data == MOUSE_X_AXIS){
      binding->gInput->curValue = data->x;
    }else if(binding->action_data == MOUSE_Y_AXIS){
      binding->gInput->curValue = data->y;
    }
    fire_event(&newEvent);
  }else if(curEvent->event_type == ENGINE_MOUSE_WHEEL){
    mouse_wheel_data* data = (mouse_wheel_data*)curEvent->event_data;
    binding->gInput->curValue = data->direction;
    fire_event(&newEvent);
  }
  return FALSE;
}

b8 initiate_input_system(input_state* state){
  state->input_mappings = darray_new(sizeof(game_input*));
  state->input_bindings = darray_new(sizeof(input_binding*));
  return TRUE;
}

b8 register_input_mapping(input_state* state, const char* input_name, u32* out_id){
  static u64 new_id = 0;
  game_input** gInputs = (game_input**)state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(vstrcmp(gInputs[i]->name, input_name) == TRUE){
      return FALSE;
    }
  }
  game_input gInput = {
    .id = new_id,
    .name = input_name,
    .curValue = 0,
  };
  darray_push(state->input_mappings, &gInput);
  new_id++;
  return TRUE;
}

game_input* get_input_mapping(input_state* state, u64 mapping_id){
  game_input** curMaps = state->input_mappings->data;
  for(int i = 0; i < state->input_mappings->length; i++){
    if(curMaps[i]->id == mapping_id){
      return curMaps[i];
    }
  }
  return NULL;
}

input_binding* get_input_binding(input_state* state, u64 action_id, u64 mapping_id){
  input_binding** curActions = state->input_bindings->data;
  for(int i = 0; i < state->input_bindings->length; i++){
    if(curActions[i]->action_id == action_id && curActions[i]->gInput->id == mapping_id){
      return curActions[i];
    }
  }
  return NULL;
}

b8 bind_input_action(input_state* state, u64 mapping_id, event_id action_id, i8 valueToMap){
  game_input* mapping = get_input_mapping(state, mapping_id);
  if(mapping == NULL){
    return FALSE;
  }
  input_binding* binding = get_input_binding(state, action_id, mapping_id);
  if(binding == NULL){
    input_binding* newBinding = vallocate(sizeof(input_binding), MEMORY_TAG_INPUT_DATA);
    newBinding->action_id = action_id;
    newBinding->valueToMap = valueToMap;
    newBinding->influencingInputValue = FALSE;
    newBinding->gInput = mapping;
    newBinding->listener_id = register_listener(action_id, action_handler, newBinding);
    darray_push(state->input_bindings, &newBinding);
    return TRUE;
  }
  return FALSE;
}

b8 unbind_input_action(input_state* state, u64 mapping_id, event_id action_id){
  input_binding** curActions = state->input_bindings->data;
  for(int i = 0; i < state->input_bindings->length; i++){
    if(curActions[i]->action_id == action_id && curActions[i]->gInput->id == mapping_id){
      darray_delete(state->input_bindings, i);
      return TRUE;
    }
  }
  return FALSE;
}

b8 shutdown_input_sytem(input_state *state){
  darray_free(state->input_mappings);
  darray_free(state->input_bindings);
  return TRUE;
}
