#pragma once
#include "defines.h"
#include "ecs/ecs_api.h"
#include "ecs/vtransform.h"

typedef struct _vcamera{
  b8 active;
  vtransform *transform;
}vcamera;

VECS_API b8 register_camera(u64 entityID, b8 active);

VECS_API vcamera* camera_get_active();