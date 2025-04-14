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
  f32 xy[3];
} vec2;

vec3 multiply_scalar(vec3 vector, f32 scalar);