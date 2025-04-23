#include "vmath.h"

vec3 multiply_scalar(vec3 vector, f32 scalar){
  vec3 ret_vec;
  for(int i = 0; i < 3; i++){
    ret_vec.xyz[i] = vector.xyz[i] * scalar;
  }
  return ret_vec;
}

mat4 identity_matrix(){
  mat4 ret_matrix = {0};
  for(int i = 0; i < 4; i++){
    ret_matrix.mat[i*4+i] = 1;
  }
  return ret_matrix;
}

mat4 translation_matrix(vec3 translation){
  mat4 ret_matrix = identity_matrix();
  ret_matrix.a14 = translation.x;
  ret_matrix.a24 = translation.y;
  ret_matrix.a34 = translation.z;
  return ret_matrix;
}

mat4 scaling_matrix(vec3 scale){
  mat4 ret_matrix = identity_matrix();
  for(int i = 0; i < 4; i++){
    ret_matrix.mat[i*4+i] = scale.xyz[i];
  }
  return ret_matrix;
}

mat4 rotation_matrix(quat quaternion){
  mat4 ret_matrix = identity_matrix();
  f32 xx = quaternion.x*quaternion.x;
  f32 xy = quaternion.x*quaternion.y;
  f32 xz = quaternion.x*quaternion.z;
  f32 xw = quaternion.x*quaternion.w;
  
  f32 yy = quaternion.y*quaternion.y;
  f32 yz = quaternion.y*quaternion.z;
  f32 yw = quaternion.y*quaternion.w;
  
  f32 zz = quaternion.z*quaternion.z;
  f32 zw = quaternion.z*quaternion.w;

  ret_matrix.mat[0]  = 1 - 2 * ( yy + zz );
  ret_matrix.mat[1]  =     2 * ( xy - zw );
  ret_matrix.mat[2]  =     2 * ( xz + yw );
  ret_matrix.mat[4]  =     2 * ( xy + zw );
  ret_matrix.mat[5]  = 1 - 2 * ( xx + zz );
  ret_matrix.mat[6]  =     2 * ( yz - xw );
  ret_matrix.mat[8]  =     2 * ( xz - yw );
  ret_matrix.mat[9]  =     2 * ( yz + xw );
  ret_matrix.mat[10] = 1 - 2 * ( xx + yy );

  return ret_matrix;
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