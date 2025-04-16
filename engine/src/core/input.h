#pragma once

#include "defines.h"
#include "container/darray.h"

/*!
 * @brief A struct that abstracts away hardware input
 */
typedef struct _input_action{
  u64 action_id; /*The id that maps to the code that's spit out by the platform layer hardware */
  i8 valueToMap;/*The value that the input mapping will be set to when this is pressed or changed in some way, this is unused for axis hardware like joysticks */
  i8 curValue;/*The current value of the button or axis */
  u64 listener_id[10]; /*An array of the event listener ids that keeps this value updated*/
  u8 listener_count; /*The number of listener ids that are registered*/
} input_action;

/*!
 * @brief A struct that contains the information that the engine and game will use for the input system 
 */
typedef struct _input_mapping{
  const char* name; /*The plaintext name that can be referenced in the code itself to query state or get events about*/
  u64 id; /*An id that can be used to query state or get events, theoretically the name should be used to get this value*/
  i8 value; /*The current value of the mapping.*/
  darray* actions; /*The maximum value in this dynamic array (up to 1) is then subtracted by the maximum value of negative mappings and put in value */
} input_mapping;

typedef struct _input_state{
  darray* input_mappings;
} input_state;

b8 init_input_system(input_state* state);

/**
 * @brief Register a front facing action for the game to keep track of
 * @param state A pointer to the input state to insert the new input mapping
 * @param input_name A plaintext name for the action, this should be unique
 * @param out_id the ID number assigned to the action, this is how the game should interact with this action in this system
 * @return TRUE if the input was successfully mapped, FALSE if this input_name has already been used
 */
VAPI b8 register_input_mapping(input_state* state, const char* input_name, u32* out_id);

/**
 * @brief Binds a hardware input to a game input, if the action_id is already bound to the mapping_id than the input action value is updated
 * @param state A pointer to the input state that the action is being bound in
 * @param mapping_id The ID of the input mapping
 * @param action_id The hardware ID to map
 * @param action_value The value that the action has when activated
 * @param is_positive_mapping Whether the hardware action should be added or subtracted from the value of the input_mapping
 * @return TRUE if the input action was successfully bound, FALSE if the mapping_id doesn't exist.
 */
VAPI b8 bind_input_action(input_state* state, u32 mapping_id, u64 action_id, u8 action_value, b8 is_positive_mapping);

b8 shutdown_input_sytem(input_state* state);