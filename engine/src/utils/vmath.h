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

typedef mat4 mat4x4;

typedef union _mat2x4{
  struct {
    f32 a11;
    f32 a21;
    f32 a12;
    f32 a22;
    f32 a13;
    f32 a23;
    f32 a14;
    f32 a24;
  };
  f32 mat[8];
} mat2x4;

typedef union _mat4x2{
  struct {
    f32 a11;
    f32 a21;
    f32 a31;
    f32 a41;
    f32 a12;
    f32 a22;
    f32 a32;
    f32 a42;
  };
  f32 mat[8];
} mat4x2;

vec3 multiply_scalar(vec3 vector, f32 scalar);

b8 identity_matrix(u64 rowsCols, f32* outMatrix);

mat4 translation_matrix(vec3 translation);
mat4 scaling_matrix(vec3 scale);
mat4 rotation_matrix(quat quaternion);

/**
 * @brief Multiply two arbitrary matrices together, the ordering is not arbitrary however
 * @param mat1 A pointer to the f32 array for the first matrix
 * @param mat1Rows How many rows in the first matrix
 * @param mat1Cols How many columns in the first matrix
 * @param mat2 A pointer to the f32 array for the second matrix
 * @param mat2Rows How many rows in the seconds matrix
 * @param mat2Cols How many columns in the second matrix
 * @param outMatrix The zero'd out matrix to fill with the result. This function doesn't check it but it must be  mat1Rowsxmat2Cols size
 * @return FALSE if mat1Cols and mat2Rows aren't equal, otherwise TRUE
 */
b8 matrix_multiply(f32* mat1, u64 mat1Rows, u64 mat1Cols, f32* mat2, u64 mat2Rows, u64 mat2Cols, f32* outMatrix);

/**
 * @brief Shorthand for multiplying a matrix with a vector.
 * @param mat The matrix to multiply with the vector. This matrix needs to be vecLengthxvecLength in dimensions
 * @param vec The vector to multiply with the matrix.
 * @param vecLength The number of elements in the vector
 * @param outVec The vector to put the results into
 */
void matrix_vec_multiply(f32* mat, f32* vec, u64 vecLength, f32* outVec);

/**
 * @brief Type casting for multiplying a 4x4 matrix with a 4 member vector
 * @param mat A 4x4 matrix passed by reference
 * @param vec A 4 member vector passed by reference
 * @param outVec A pointer to the vector with the results
 */
void matrix_vec4_multiply(mat4x4 mat, vec4 vec, vec4* outVec);

/*! 
 * @brief Clamps the value between the minimum and maximum
 * @param value An interger to be contained
 * @param minimum The minimum that value is allowed to be
 * @param maximum The maximum that value is allowed to be
 * @return value if the value is between minimum and maximum, minimum if value is smaller than minimum and likewise with maximum
 */
u64 vclamp(u64 value, u64 minimum, u64 maximum);