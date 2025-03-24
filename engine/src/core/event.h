#pragma once

#include "defines.h"

typedef struct _event{
  u64 event_type;
  void* event_data;
} event;