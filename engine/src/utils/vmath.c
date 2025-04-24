#include "vmath.h"

vec3 multiply_scalar(vec3 vector, f32 scalar){
  vec3 ret_vec;
  for(int i = 0; i < 3; i++){
    ret_vec.xyz[i] = vector.xyz[i] * scalar;
  }
  return ret_vec;
}

b8 identity_matrix(u64 rowsCols, f32* outMatrix){
  for(int i = 0; i < rowsCols; i++){
    for(int j = 0; j < rowsCols; j++){
      outMatrix[i*rowsCols+j] = i==j?1:0;
    }
  }
  return TRUE;
}

mat4 translation_matrix(vec3 translation){
  mat4 ret_matrix;
  identity_matrix(4, ret_matrix.mat);
  ret_matrix.a14 = translation.x;
  ret_matrix.a24 = translation.y;
  ret_matrix.a34 = translation.z;
  return ret_matrix;
}

mat4 scaling_matrix(vec3 scale){
  mat4 ret_matrix = {0};
  for(int i = 0; i < 3; i++){
    ret_matrix.mat[i*4+i] = scale.xyz[i];
  }
  ret_matrix.mat[15] = 1;
  return ret_matrix;
}

mat4 rotation_matrix(quat quaternion){
  mat4 ret_matrix = {0};
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
  ret_matrix.mat[15] = 1;

  return ret_matrix;
}

b8 matrix_multiply(f32* mat1, u64 mat1Rows, u64 mat1Cols, f32* mat2, u64 mat2Rows, u64 mat2Cols, f32* outMatrix){
  if(mat1Cols != mat2Rows){
    return FALSE;
  }
  for(int mat1RowIndex = 0; mat1RowIndex < mat1Rows; mat1RowIndex++){
    for(int mat2ColIndex = 0; mat2ColIndex< mat2Cols; mat2ColIndex++){
      outMatrix[(mat1RowIndex*mat2Cols)+mat2ColIndex] = 0;
      for(int i = 0; i < mat1Cols; i++){
        /*u64 m1Index = (mat1RowIndex*mat1Cols)+i;
        u64 m2Index = (i*mat2Cols)+mat2ColIndex;
        u64 outIndex = (mat1RowIndex*mat2Cols)+mat2ColIndex;
        f32 mat1Value = mat1[m1Index];
        f32 mat2Value = mat2[m2Index];
        f32 outMatrixvalue = mat1Value * mat2Value;*/
        outMatrix[(mat1RowIndex*mat2Cols)+mat2ColIndex] += mat1[(mat1RowIndex*mat1Cols)+i] * mat2[(i*mat2Cols)+mat2ColIndex];
      }
    }
  }
  return TRUE;
}

void matrix_vec_multiply(f32* mat, f32* vec, u64 vecLength, f32* outVec){
  matrix_multiply(mat, vecLength, vecLength, vec, vecLength, 1, outVec);
}

void matrix_vec4_multiply(mat4x4 mat, vec4 vec, vec4* outVec){
  matrix_vec_multiply(mat.mat, vec.xyzw, 4, outVec->xyzw);
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