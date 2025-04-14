#include "event.h"
#include "container/darray.h"
#include "container/map.h"
#include "core/vmemory.h"
#include "platform/platform.h"

// typedef b8 (*event_listener)(event* event);

typedef struct _event_listener_data{
  u64 listen_id;
  u64 event_id;
  void* state;
  event_listener function;
} event_listener_data;

static darray* event_queue = NULL;
static darray* event_listeners = NULL;
static u64 listener_id = 0;

void initiate_event_system(){
  event_queue = darray_new(sizeof(event));
  event_listeners = darray_new(sizeof(event_listener_data));
}

void shutdown_event_system(){
  darray_free(event_queue);
  darray_free(event_listeners);
}

u64 register_listener(u64 e_type, event_listener func, void* state){
  u64 ret_id = listener_id;
  listener_id++;
  event_listener_data dat = {
    .listen_id = ret_id,
    .event_id = e_type,
    .state = state,
    .function = func,
  };
  darray_push(event_listeners, &dat);
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
      VWARN("Events are being fired without any listeners");
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
  if(event_listeners->length == 0){
    return FALSE;
  }
  event_listener_data* listeners = (event_listener_data*)event_listeners->data;
  for(int i = 0; i < event_listeners->length; i++){
    if(listeners[i].event_id != new_event->event_type){
      continue;
    }
    if(listeners[i].function(new_event, listeners[i].state)){
      break;
    }
  }
  vfree(new_event->event_data, new_event->event_data_size, MEMORY_TAG_EVENT_DATA);
  return TRUE;
}

b8 deregister_listener(u64 listener_id){
  if(event_listeners->length == 0){
    return FALSE;
  }
  event_listener_data* listeners = (event_listener_data*)event_listeners->data;
  for(int i = 0; i < event_listeners->length; i++){
    if(listeners[i].listen_id == listener_id){
      darray_delete(event_listeners, i);
      return TRUE;
    }
  }
  return FALSE;
}
