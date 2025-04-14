#include "vmath.h"

vec3 multiply_scalar(vec3 vector, f32 scalar){
  vec3 ret_vec;
  for(int i = 0; i < 3; i++){
    ret_vec.xyz[i] = vector.xyz[i] * scalar;
  }
  return ret_vec;
}