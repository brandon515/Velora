#include "event.h"
#include "container/darray.h"

darray* event_queue;
darray* event_listeners;

void initiate_event_system(){
  event_queue = darray_new(sizeof(event));
  event_listeners = darray_new(sizeof(event_listener**));
}

void shutdown_event_system(){
  //
}

u64 register_listener(u64 e_type, event_listener func){
  //
}

b8 queue_event(event* new_event){
  //
}

b8 fire_event(event* new_event){
  //
}

b8 deregister_listener(u64 e_type, u64 listener_id){
  //
}
