#pragma once

#include "defines.h"

typedef struct _event{
  u64 event_type;
  u64 event_data_size;
  void* event_state;
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
  ENGINE_INPUT_BUTTON,
  ENGINE_MOUSE_POSITION,
  ENGINE_MOUSE_WHEEL,
  ENGINE_MOUSE_BUTTON,
  ENGINE_EVENT_ID_END,
} event_id;

/*!
 * @brief Event listener function pointer
 * @param event A pointer to the event to be processed
 * @return TRUE if the event is consumed by the listener, FALSE if the even should be pushed to other listeners
 */
typedef b8 (*event_listener)(event* event);

VAPI void initiate_event_system();
VAPI void shutdown_event_system();

/*!
 * @brief Registers a listener to a specfic event
 * @param e_type  A u64 that signifies the type of event the listener wants to get
 * @param func  A function pointer of event_listener type that will process the event
 * @result The listenerID associated with that event. This is used to remove the listener later if needed
 */
VAPI u64 register_listener(u64 e_type, event_listener func, void* state);

/*!
 * @brief Copies the data in the pointer to the dynamic array and queues up an event to be dispatched in the main loop of the engine
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

// Engine event data structs

typedef struct _resize_data{
  u64 width;
  u64 height;
} resize_data;

typedef struct _button_data{
  u8 pressed;
  u64 button_code;
} button_data;

typedef struct _mouse_position_data{
  i32 x;
  i32 y;
} mouse_position_data;

typedef struct _mouse_wheel_data{
  i8 direction;
} mouse_wheel_data;


typedef enum _mouse_button{
  MOUSE_L_BUTTON,
  MOUSE_R_BUTTON,
  MOUSE_M_BUTTON,
  MOUSE_X1_BUTTON,
  MOUSE_X2_BUTTON,
} mouse_button;

typedef struct _mouse_button_data{
  u8 pressed;
  mouse_button button_code;
  i32 x;
  i32 y;
} mouse_button_data;