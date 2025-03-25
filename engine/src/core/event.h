#pragma once

#include "defines.h"

typedef struct _event{
  u64 event_type;
  void* event_data;
} event;

/* We mark the end of the event_id enum so that the game can create their own
 * custom event ids without trampling over the internal engine events. The game
 * judt needs to set the first enum variable to ENGINE_EVENT_ID_END and it'll be
 * good since ENGINE_EVENT_ID_END is unused.
 */
typedef enum _event_id{
  ENGINE_INPUT_KEYBOARD,
  ENGINE_EVENT_ID_END,
} event_id;

typedef b8 (*event_listener)(event* event);

void initiate_event_system();
void shutdown_event_system();

/* 
 * @function  register_listener
 * @abstract  Registers a listener to a specfic event
 * @param e_type  A u64 that signifies the type of event the listener wants to get
 * @param func  A function pointer that will process the event
 * @result The listenerID associated with that event. 
 */
VAPI u64 register_listener(u64 e_type, event_listener func);
VAPI b8 queue_event(event* new_event);
VAPI b8 fire_event(event* new_event);
VAPI b8 deregister_listener(u64 e_type, u64 listener_id);