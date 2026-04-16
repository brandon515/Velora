#pragma once
#include "defines.h"

typedef struct _vcamera{
  b8 active;
}vcamera;

b8 register_camera(u64 entityID, b8 active);