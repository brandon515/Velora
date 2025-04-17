#include "vmath.h"

vec3 multiply_scalar(vec3 vector, f32 scalar){
  vec3 ret_vec;
  for(int i = 0; i < 3; i++){
    ret_vec.xyz[i] = vector.xyz[i] * scalar;
  }
  return ret_vec;
}

u64 vclamp(u64 value, u64 minimum, u64 maximum){
  if(value < minimum){
    return minimum;
  }else if(value > maximum){
    return maximum;
  }else{
    return value;
  }
}