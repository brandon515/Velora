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

typedef union _vec4 {
  struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
  };
  f32 xyzw[4];
} vec4;

typedef vec4 quat;

/**
 * @brief The member variables are ayx where y is the row and x is the column and the origin is the upper left
 */
typedef union _mat4{
  struct {
    f32 a11;
    f32 a21;
    f32 a31;
    f32 a41;
    f32 a12;
    f32 a22;
    f32 a32;
    f32 a42;
    f32 a13;
    f32 a23;
    f32 a33;
    f32 a43;
    f32 a14;
    f32 a24;
    f32 a34;
    f32 a44;
  };
  f32 mat[16];
} mat4;

vec3 multiply_scalar(vec3 vector, f32 scalar);

mat4 identity_matrix();

mat4 translation_matrix(vec3 translation);
mat4 scaling_matrix(vec3 scale);
mat4 rotation_matrix(quat quaternion);

/*! 
 * @brief Clamps the value between the minimum and maximum
 * @param value An interger to be contained
 * @param minimum The minimum that value is allowed to be
 * @param maximum The maximum that value is allowed to be
 * @return value if the value is between minimum and maximum, minimum if value is smaller than minimum and likewise with maximum
 */
u64 vclamp(u64 value, u64 minimum, u64 maximum);