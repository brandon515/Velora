#include "vmath.h"
#include <math.h>

void vec_multiply_scalar(f32 *vecData, f32 scalar, u8 numberOfVariables, f32 *outVec){
  for(int i = 0; i < numberOfVariables; i++){
    outVec[i] = vecData[i] * scalar;
  }
}

vec2 vec2_multiply_scalar(vec2 vector, f32 scalar){
  vec2 ret_vec;
  vec_multiply_scalar(vector.xy, scalar, 2, ret_vec.xy);
  return ret_vec;
  //
}

vec3 vec3_multiply_scalar(vec3 vector, f32 scalar){
  vec3 ret_vec;
  vec_multiply_scalar(vector.xyz, scalar, 3, ret_vec.xyz);
  return ret_vec;
}

vec4 vec4_multiply_scalar(vec4 vector, f32 scalar){
  vec4 ret_vec;
  vec_multiply_scalar(vector.xyzw, scalar, 4, ret_vec.xyzw);
  return ret_vec;
}

void identity_matrix(u64 rowsCols, f32* outMatrix){
  for(int i = 0; i < rowsCols; i++){
    for(int j = 0; j < rowsCols; j++){
      outMatrix[i*rowsCols+j] = i==j?1:0;
    }
  }
  return TRUE;
}

void vec_add(f32 *vector1, f32 *vector2, u8 numberOfVariables, f32 *outVector){
  for(int i = 0; i < numberOfVariables; i++){
    outVector[i] = vector1[i] + vector2[i];
  }
}

vec2 vec2_add(vec2 vector1, vec2 vector2){
  vec2 retVec;
  vec_add(vector1.xy, vector2.xy, 2, retVec.xy);
  return retVec;
}

vec3 vec3_add(vec3 vector1, vec3 vector2){
  vec3 retVec;
  vec_add(vector1.xyz, vector2.xyz, 3, retVec.xyz);
  return retVec;
}

vec4 vec4_add(vec4 vector1, vec4 vector2){
  vec4 retVec;
  vec_add(vector1.xyzw, vector2.xyzw, 4, retVec.xyzw);
  return retVec;
}

mat2 indentity_mat2(){
  mat2 retMat;
  identity_matrix(2, retMat.mat);
  return retMat;
}

mat3 indentity_mat3(){
  mat3 retMat;
  identity_matrix(3, retMat.mat);
  return retMat;
}

mat4 indentity_mat4(){
  mat4 retMat;
  identity_matrix(4, retMat.mat);
  return retMat;
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

quat euler_to_quat(vec3 eulerDegreeAngles){
  f32 pitch = eulerDegreeAngles.y*(V_PI/180);
  f32 yaw = eulerDegreeAngles.z*(V_PI/180);
  f32 roll = eulerDegreeAngles.x*(V_PI/180);

  quat ret_quat = {0};
  ret_quat.x = (sin(roll/2) * cos(pitch/2) * cos(yaw/2)) - (cos(roll/2) * sin(pitch/2) * sin(yaw/2));
  ret_quat.y = (cos(roll/2) * sin(pitch/2) * cos(yaw/2)) + (sin(roll/2) * cos(pitch/2) * sin(yaw/2));
  ret_quat.z = (cos(roll/2) * cos(pitch/2) * sin(yaw/2)) - (sin(roll/2) * sin(pitch/2) * cos(yaw/2));
  ret_quat.w = (cos(roll/2) * cos(pitch/2) * cos(yaw/2)) + (sin(roll/2) * sin(pitch/2) * sin(yaw/2));
  return ret_quat;
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

mat4 model_matrix(vec3 translation, quat rotation, vec3 scale){
  mat4 transMatrix = translation_matrix(translation);
  mat4 rotMatrix = rotation_matrix(rotation);
  mat4 scaleMatrix = scaling_matrix(scale);
  mat4 scratchMatrix = {0};
  mat4 ret_matrix = {0};
  matrix4_multiply(rotMatrix, transMatrix, &scratchMatrix);
  matrix4_multiply(scaleMatrix, scratchMatrix, &ret_matrix);
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

void matrix_vec2_multiply(mat2x2 mat, vec2 vec, vec2* outVec){
  matrix_vec_multiply(mat.mat, vec.xy, 2, outVec->xy);
}

void matrix_vec3_multiply(mat3x3 mat, vec3 vec, vec3* outVec){
  matrix_vec_multiply(mat.mat, vec.xyz, 3, outVec->xyz);
}

void matrix_vec4_multiply(mat4x4 mat, vec4 vec, vec4* outVec){
  matrix_vec_multiply(mat.mat, vec.xyzw, 4, outVec->xyzw);
}

void matrix2_multiply(mat2x2 mat1, mat2x2 mat2, mat2x2* outMatrix){
  matrix_multiply(mat1.mat, 2, 2, mat2.mat, 2, 2, outMatrix->mat);
}

void matrix3_multiply(mat3x3 mat1, mat3x3 mat2, mat3x3* outMatrix){
  matrix_multiply(mat1.mat, 3, 3, mat2.mat, 3, 3, outMatrix->mat);
}

void matrix4_multiply(mat4x4 mat1, mat4x4 mat2, mat4x4* outMatrix){
  matrix_multiply(mat1.mat, 4, 4, mat2.mat, 4, 4, outMatrix->mat);
}

b8 matrix4_invert(mat4x4 mat, mat4x4* outMatrix)
{
  float inv[16];
  float* m = mat.mat;

  inv[0] = m[5]  * m[10] * m[15] - 
            m[5]  * m[11] * m[14] - 
            m[9]  * m[6]  * m[15] + 
            m[9]  * m[7]  * m[14] +
            m[13] * m[6]  * m[11] - 
            m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] + 
            m[4]  * m[11] * m[14] + 
            m[8]  * m[6]  * m[15] - 
            m[8]  * m[7]  * m[14] - 
            m[12] * m[6]  * m[11] + 
            m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] - 
            m[4]  * m[11] * m[13] - 
            m[8]  * m[5] * m[15] + 
            m[8]  * m[7] * m[13] + 
            m[12] * m[5] * m[11] - 
            m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] + 
              m[4]  * m[10] * m[13] +
              m[8]  * m[5] * m[14] - 
              m[8]  * m[6] * m[13] - 
              m[12] * m[5] * m[10] + 
              m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] + 
            m[1]  * m[11] * m[14] + 
            m[9]  * m[2] * m[15] - 
            m[9]  * m[3] * m[14] - 
            m[13] * m[2] * m[11] + 
            m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] - 
            m[0]  * m[11] * m[14] - 
            m[8]  * m[2] * m[15] + 
            m[8]  * m[3] * m[14] + 
            m[12] * m[2] * m[11] - 
            m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] + 
            m[0]  * m[11] * m[13] + 
            m[8]  * m[1] * m[15] - 
            m[8]  * m[3] * m[13] - 
            m[12] * m[1] * m[11] + 
            m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] - 
            m[0]  * m[10] * m[13] - 
            m[8]  * m[1] * m[14] + 
            m[8]  * m[2] * m[13] + 
            m[12] * m[1] * m[10] - 
            m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] - 
            m[1]  * m[7] * m[14] - 
            m[5]  * m[2] * m[15] + 
            m[5]  * m[3] * m[14] + 
            m[13] * m[2] * m[7] - 
            m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] + 
            m[0]  * m[7] * m[14] + 
            m[4]  * m[2] * m[15] - 
            m[4]  * m[3] * m[14] - 
            m[12] * m[2] * m[7] + 
            m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] - 
            m[0]  * m[7] * m[13] - 
            m[4]  * m[1] * m[15] + 
            m[4]  * m[3] * m[13] + 
            m[12] * m[1] * m[7] - 
            m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] + 
              m[0]  * m[6] * m[13] + 
              m[4]  * m[1] * m[14] - 
              m[4]  * m[2] * m[13] - 
              m[12] * m[1] * m[6] + 
              m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] + 
            m[1] * m[7] * m[10] + 
            m[5] * m[2] * m[11] - 
            m[5] * m[3] * m[10] - 
            m[9] * m[2] * m[7] + 
            m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] - 
            m[0] * m[7] * m[10] - 
            m[4] * m[2] * m[11] + 
            m[4] * m[3] * m[10] + 
            m[8] * m[2] * m[7] - 
            m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] + 
              m[0] * m[7] * m[9] + 
              m[4] * m[1] * m[11] - 
              m[4] * m[3] * m[9] - 
              m[8] * m[1] * m[7] + 
              m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] - 
            m[0] * m[6] * m[9] - 
            m[4] * m[1] * m[10] + 
            m[4] * m[2] * m[9] + 
            m[8] * m[1] * m[6] - 
            m[8] * m[2] * m[5];

  float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det <= 0.00000001f && det >= -0.00000001f){
    return FALSE;
  }

  det = 1.0 / det;

  for (int i = 0; i < 16; i++){
    outMatrix->mat[i] = inv[i] * det;
  }

  return TRUE;
}

b8 perspective_matrix(f32 near, f32 far, f32 left, f32 right, f32 top, f32 bottom, mat4x4* outMatrix){
  (*outMatrix) = (mat4x4){0};
  if(
    (right-left) == 0 ||
    (top-bottom) == 0 ||
    (near-far) == 0
  ){
    return FALSE;
  }
  outMatrix->a11 = (2*near)/(right-left);
  outMatrix->a22 = (2*near)/(top-bottom);
  outMatrix->a13 = (right+left)/(right-left);
  outMatrix->a23 = (top+bottom)/(top-bottom);
  outMatrix->a33 = (far+near)/(near-far);
  outMatrix->a43 = -1;
  outMatrix->a34 = (2*far*near)/(near-far);
  return TRUE;
}

b8 orthographic_matrix(f32 near, f32 far, f32 left, f32 right, f32 top, f32 bottom, mat4x4* outMatrix){
  (*outMatrix) = (mat4x4){0};
  if(
    (right-left) == 0 ||
    (top-bottom) == 0 ||
    (far-near) == 0
  ){
    return FALSE;
  }
  outMatrix->a11 = 2/(right-left);
  outMatrix->a22 = 2/(top-bottom);
  outMatrix->a33 = -2/(far-near);
  outMatrix->a14 = -((right+left)/(right-left));
  outMatrix->a24 = -((top+bottom)/(top-bottom));
  outMatrix->a34 = -((far+near)/(far-near));
  outMatrix->a44 = 1;
  return TRUE;
}

mat4 perspective_projection_matrix(f32 nearClip, f32 farClip, f32 degreeView, f32 aspectRatio){
  f32 radianView = degreeView*(V_PI/180);
  f32 top = tan(radianView/2)*nearClip;
  f32 bottom = -top;
  f32 right = aspectRatio * top;
  f32 left = -right;
  mat4x4 ret_matrix;
  perspective_matrix(nearClip, farClip, left, right, top, bottom, &ret_matrix);
  return ret_matrix;
}

mat4 orthographic_projection_matrix(f32 nearClip, f32 farClip, f32 topDistance, f32 aspectRatio){
  f32 bottom = -topDistance;
  f32 right = aspectRatio * topDistance;
  f32 left = -right;
  mat4x4 ret_matrix;
  orthographic_matrix(nearClip, farClip, left, right, topDistance, bottom, &ret_matrix);
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

quat rotation_to_quat(mat4 rotMat){
  f32 trace = 1 + rotMat.mat[0] + rotMat.mat[5] + rotMat.mat[10];
  quat retQuat;
  if(trace > 0.00000001f){
    f32 S = sqrt(trace)*2;
    retQuat.x = (rotMat.mat[9] - rotMat.mat[6]) / S;
    retQuat.y = (rotMat.mat[2] - rotMat.mat[8]) / S;
    retQuat.z = (rotMat.mat[4] - rotMat.mat[1]) / S;
    retQuat.w = 0.25f * S;
    return retQuat;
  }

  if(rotMat.mat[0] > rotMat.mat[5] && rotMat.mat[0] > rotMat.mat[10]){
    f32 S = sqrt(1.0f + rotMat.mat[0] - rotMat.mat[5] - rotMat.mat[10]) * 2;
    retQuat.x = 0.25f * S;
    retQuat.y = (rotMat.mat[4] + rotMat.mat[1]) / S;
    retQuat.z = (rotMat.mat[2] + rotMat.mat[8]) / S;
    retQuat.w = (rotMat.mat[9] + rotMat.mat[6]) / S;
  }else if(rotMat.mat[5] > rotMat.mat[10]){
    f32 S = sqrt(1.0f + rotMat.mat[5] - rotMat.mat[0] - rotMat.mat[10]) * 2;
    retQuat.x = (rotMat.mat[4] + rotMat.mat[1]) / S;
    retQuat.y = 0.25f * S;
    retQuat.z = (rotMat.mat[9] + rotMat.mat[6]) / S;
    retQuat.w = (rotMat.mat[2] + rotMat.mat[8]) / S;
  }else{
    f32 S = sqrt(1.0f + rotMat.mat[10] - rotMat.mat[0] - rotMat.mat[5]) * 2;
    retQuat.x = (rotMat.mat[2] + rotMat.mat[8]) / S;
    retQuat.y = (rotMat.mat[9] + rotMat.mat[6]) / S;
    retQuat.z = 0.25f * S;
    retQuat.w = (rotMat.mat[4] + rotMat.mat[1]) / S;
  }
  return retQuat;
}

void decompose_model_matrix(mat4 matrix, vec3* out_translation, quat* out_quat, vec3* out_scale){
  out_translation->x = matrix.a14;
  out_translation->y = matrix.a24;
  out_translation->z = matrix.a34;

  matrix.a14 = 0;
  matrix.a24 = 0;
  matrix.a34 = 0;

  out_scale->x = sqrt((matrix.a11*matrix.a11)+(matrix.a21*matrix.a21)+(matrix.a31*matrix.a31));
  out_scale->y = sqrt((matrix.a12*matrix.a12)+(matrix.a22*matrix.a22)+(matrix.a32*matrix.a32));
  out_scale->z = sqrt((matrix.a13*matrix.a13)+(matrix.a23*matrix.a23)+(matrix.a33*matrix.a33));

  for(int i = 0; i < 3; i++){
    matrix.mat[(0*4)+i] /= out_scale->x;
    matrix.mat[(1*4)+i] /= out_scale->y;
    matrix.mat[(2*4)+i] /= out_scale->z;
  }

  (*out_quat) = rotation_to_quat(matrix);
}