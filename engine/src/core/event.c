#include "event.h"
#include "container/darray.h"
#include "container/map.h"
#include "platform/platform.h"

// typedef b8 (*event_listener)(event* event);

typedef struct _event_listener_data{
  u64 id;
  event_listener function;
} event_listener_data;

static darray* event_queue = NULL;
static int_map* event_listeners = NULL;
static u64 listener_id = 0;

void initiate_event_system(){
  event_queue = darray_new(sizeof(event));
  event_listeners = map_new(sizeof(darray*));
  for(int i = ENGINE_INPUT_KEYBOARD; i < ENGINE_EVENT_ID_END; i++){
    darray* temp = darray_new(sizeof(event_listener_data));
    map_set_item(event_listeners, i, &temp);
  }
}

void shutdown_event_system(){
  darray_free(event_queue);
  darray** dat = (darray**)event_listeners->data->data;
  for(int i = 0; i < event_listeners->data->length; i++){
    darray_free(dat[i]);
  }
  map_free(event_listeners);
}

u64 register_listener(u64 e_type, event_listener func){
  darray* listener_array;
  if(map_get_item(event_listeners, e_type, &listener_array) == FALSE){
    listener_array = darray_new(sizeof(event_listener_data));
    map_set_item(event_listeners, e_type, &listener_array);
  }
  u64 ret_id = listener_id;
  listener_id++;
  event_listener_data dat = {
    .id = ret_id,
    .function = func,
  };
  darray_push(listener_array, &dat);
  return ret_id;
}

b8 queue_event(event* new_event){
  darray_push(event_queue, new_event);
  return TRUE;
}

b8 pump_events(f64 time_limit){
  f64 start_time = platform_get_absolute_time();
  event* queue = (event*)event_queue->data;
  for(int i = 0; i < event_queue->length; i++){
    if(fire_event(&queue[i]) == FALSE){
      VWARN("Event with id %d is being fired without any listeners", queue[i].event_type);
    }
    f64 benchmark_time = platform_get_absolute_time();
    f64 duration = benchmark_time - start_time;
    if(duration > time_limit){
      VWARN("Event queue was not able to be cleared out in time");
      darray_drain(event_queue, i);
      return FALSE;
    }
  }
  darray_clear(event_queue);
  return TRUE;
}

b8 fire_event(event* new_event){
  darray* listener_array;
  if(map_get_item(event_listeners, new_event->event_type, &listener_array) == FALSE){
    listener_array = darray_new(sizeof(event_listener_data));
    map_set_item(event_listeners, new_event->event_type, &listener_array);
    return FALSE;
  }
  if(listener_array->length == 0){
    return FALSE;
  }
  event_listener_data* listeners = (event_listener_data*)listener_array->data;
  for(int i = 0; i < listener_array->length; i++){
    if(listeners[i].function(new_event)){
      break;
    }
  }
  return TRUE;
}

b8 deregister_listener(u64 e_type, u64 listener_id){
  darray* listener_array;
  if(map_get_item(event_listeners, e_type, &listener_array) == FALSE){
    return FALSE;
  }
  if(listener_array->length == 0){
    return FALSE;
  }
  event_listener_data* listeners = (event_listener_data*)listener_array->data;
  for(int i = 0; i < listener_array->length; i++){
    if(listeners[i].id == listener_id){
      darray_delete(listener_array, i);
      return TRUE;
    }
  }
  return FALSE;
}
