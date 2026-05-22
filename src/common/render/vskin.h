#pragma once

#include "defines.h"
#include "core/utils/vmath.h"

typedef struct _vskin{
  u64 jointCount;
  mat4* jointTransform;
  mat4* inverseBindMatrix;
}vskin;