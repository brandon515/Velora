#include "event.h"
#include "container/darray.h"
#include "container/map.h"

// typedef b8 (*event_listener)(event* event);


static darray* event_queue = NULL;
static int_map* event_listeners = NULL;

void initiate_event_system(){
  event_queue = darray_new(sizeof(event));
  event_listeners = map_new(sizeof(darray));
  for(int i = ENGINE_INPUT_KEYBOARD; i < ENGINE_EVENT_ID_END; i++){
    darray* temp = darray_new(sizeof(event_listener));
    map_set_item(event_listeners, i, temp);
  }
}

//TODO: This is probably a memory leak. Look at it closer later
void shutdown_event_system(){
  darray_free(event_queue);
  map_free(event_listeners);
}

u64 register_listener(u64 e_type, event_listener func){
  darray* listener_array = darray_new(sizeof(event_listener));
  if(map_get_item(event_listeners, e_type, listener_array) == FALSE){
    map_set_item(event_listeners, e_type, listener_array);
  }
  u64 ret_id = listener_array->length;
  darray_push(listener_array, func);
  return ret_id;
}

b8 queue_event(event* new_event){
  return TRUE;
}

b8 fire_event(event* new_event){
  darray* listener_array = darray_new(sizeof(event_listener));
  if(map_get_item(event_listeners, new_event->event_type, listener_array) == FALSE){
    map_set_item(event_listeners, new_event->event_type, listener_array);
  }
  event_listener* listeners = (event_listener*)listener_array->data;
  for(int i = 0; i < listener_array->length; i++){
    if(listeners[i](new_event)){
      break;
    }
  }
  return TRUE;
}

b8 deregister_listener(u64 e_type, u64 listener_id){
  return TRUE;
}
