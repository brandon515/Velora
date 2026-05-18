#pragma once
#include "defines.h"
#include "ecs/vtransform.h"

typedef struct _vcamera{
  b8 active;
  vtransform *transform;
}vcamera;

b8 register_camera(u64 entityID, b8 active);

vcamera* camera_get_active();