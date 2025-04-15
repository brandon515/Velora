#pragma once

#include "defines.h"
#include "container/darray.h"

typedef struct _input_action{
  u64 action_id;
} input_action;

typedef struct _input_mapping{
  i8 value;

} input_mapping;

typedef struct _input_state{
  darray input_mappings;
} input_state;
