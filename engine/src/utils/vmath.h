#pragma once

#include "defines.h"

typedef union _vec3{
  struct {
    f32 x;
    f32 y;
    f32 z;
  };
  f32 xyz[3];
} vec3;

typedef union _vec2{
  struct {
    f32 x;
    f32 y;
  };
  f32 xy[2];
} vec2;

vec3 multiply_scalar(vec3 vector, f32 scalar);

/*! 
 * @brief Clamps the value between the minimum and maximum
 * @param value An interger to be contained
 * @param minimum The minimum that value is allowed to be
 * @param maximum The maximum that value is allowed to be
 * @return value if the value is between minimum and maximum, minimum if value is smaller than minimum and likewise with maximum
 */
u64 vclamp(u64 value, u64 minimum, u64 maximum);