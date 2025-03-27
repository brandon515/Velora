#pragma once

#include "defines.h"

typedef struct _event{
  u64 event_type;
  u64 event_data_size;
  void* event_data;
} event;

/* We mark the end of the event_id enum so that the game can create their own
 * custom event ids without trampling over the internal engine events. The game
 * judt needs to set the first enum variable to ENGINE_EVENT_ID_END and it'll be
 * good since ENGINE_EVENT_ID_END is unused.
 */
typedef enum _event_id{
  ENGINE_CLOSE_GAME,
  ENGINE_WINDOW_RESIZE,
  ENGINE_INPUT_KEYBOARD,
  ENGINE_EVENT_ID_END,
} event_id;

typedef b8 (*event_listener)(event* event);

VAPI void initiate_event_system();
VAPI void shutdown_event_system();

/*!
 * @brief Registers a listener to a specfic event
 * @param e_type  A u64 that signifies the type of event the listener wants to get
 * @param func  A function pointer of event_listener type that will process the event
 * @result The listenerID associated with that event. This is used to remove the listener later if needed
 */
VAPI u64 register_listener(u64 e_type, event_listener func);

/*!
 * @brief Queues up an event to be dispatched in the main loop of the engine
 * @param new_event  An event struct containing the data needed to process the event
 * @result Returns TRUE if the event was able to be queued, FALSE if not
 */
VAPI b8 queue_event(event* new_event);

/*!
 * @brief Fires an event off to the listeners that have subscribed to it RIGHT NOW
 * @param new_event  An event struct containing the data needed to process the event
 * @result Returns FALSE if no listener is available for the event fired, retruns TRUE otherwise
 */
VAPI b8 fire_event(event* new_event);

/*!
 * @brief deletes listener from the lineup
 * @param listener_id A u64 that was given when the listener asked to listen to the event
 * @result Returns FALSE if the listener has already been removed, TRUE if the listener was successfully detached
 */
VAPI b8 deregister_listener(u64 listener_id);

/*!
 * @brief Works through the event queue within the time limit. The time limit may go over by a bit, the function won't stop processing an event only between events
 * @param time_limit A float in seconds with the time limit for the event system to work through the queue
 * @result Returns FALSE if the queue wasn't able to be emptied, TRUE if it was
 */
b8 pump_events(f64 time_limit);