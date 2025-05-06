#pragma once

#include "defines.h"

#define V_PI 3.141592

typedef union _vec2{
  struct {
    f32 x;
    f32 y;
  };
  f32 xy[2];
} vec2;

typedef union _vec3{
  struct {
    f32 x;
    f32 y;
    f32 z;
  };
  f32 xyz[3];
} vec3;

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
typedef union _mat2{
  struct {
    f32 a11;
    f32 a21;
    f32 a12;
    f32 a22;
  };
  f32 mat[4];
} mat2;

typedef mat2 mat2x2;

typedef union _mat3{
  struct {
    f32 a11;
    f32 a21;
    f32 a31;
    f32 a12;
    f32 a22;
    f32 a32;
    f32 a13;
    f32 a23;
    f32 a33;
  };
  f32 mat[9];
} mat3;

typedef mat3 mat3x3;

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
 * @brief Type casting for multiplying a 2x2 matrix with a 4 member vector
 * @param mat A 2x2 matrix passed by reference
 * @param vec A 2 member vector passed by reference
 * @param outVec A pointer to the vector with the results
 */
void matrix_vec2_multiply(mat2x2 mat, vec2 vec, vec2* outVec);

/**
 * @brief Type casting for multiplying a 2x2 matrix with another
 * @param mat1 A 2x2 matrix passed by reference
 * @param mat2 A 2x2 matrix passed by reference
 * @param outMatrix A pointer to the vector with the results
 */
void matrix2_multiply(mat2x2 mat1, mat2x2 mat2, mat2x2* outMatrix);

/**
 * @brief Type casting for multiplying a 3x3 matrix with a 4 member vector
 * @param mat A 3x3 matrix passed by reference
 * @param vec A 3 member vector passed by reference
 * @param outVec A pointer to the vector with the results
 */
void matrix_vec3_multiply(mat3x3 mat, vec3 vec, vec3* outVec);

/**
 * @brief Type casting for multiplying a 3x3 matrix with another
 * @param mat1 A 3x3 matrix passed by reference
 * @param mat2 A 3x3 matrix passed by reference
 * @param outMatrix A pointer to the vector with the results
 */
void matrix3_multiply(mat3x3 mat1, mat3x3 mat2, mat3x3* outMatrix);

/**
 * @brief Type casting for multiplying a 4x4 matrix with a 4 member vector
 * @param mat A 4x4 matrix passed by reference
 * @param vec A 4 member vector passed by reference
 * @param outVec A pointer to the vector with the results
 */
void matrix_vec4_multiply(mat4x4 mat, vec4 vec, vec4* outVec);

/**
 * @brief Type casting for multiplying a 4x4 matrix with another
 * @param mat1 A 4x4 matrix passed by reference
 * @param mat2 A 4x4 matrix passed by reference
 * @param outMatrix A pointer to the vector with the results
 */
void matrix4_multiply(mat4x4 mat1, mat4x4 mat2, mat4x4* outMatrix);

/**
 * @brief Shorthand to combine the translation, rotation and scale matrix into one
 * @param translation A vector containined the translation required on the x, y, and z axies
 * @param rotation A quaternion containing the rotation of the object
 * @param scale A vector containined the scale of the object in the x, y, and z axies
 * @return A pass by reference model matrix
 */
mat4 model_matrix(vec3 translation, quat rotation, vec3 scale);

/**
 * @brief This is ripped straight from MESA GLU implementation, black magic that is
 * @param mat The matrix to be inverted
 * @param outMatrix A pointer to the matrix to store the results
 * @return TRUE if the matrix was succesfully inverted, FALSE if the matrix determinent is 0 so no inverse exists
 */
b8 matrix4_invert(mat4x4 mat, mat4x4* outMatrix);

/**
 * @brief Creates a perspective projection matrix
 * @param near The distance from the camera that the camera starts rendering
 * @param far The distance from the camera that the camera stops rendering
 * @param left The length from the center of the extent to the left edge
 * @param right The length from the center of the extent to the right edge
 * @param top The length from the center of the extent to the top edge
 * @param bottom The length from the center of the extent to the bottom edge
 * @param outMatrix A pointer to the matrix to store the output
 * @return FALSE if left = right, top = bottom or near = far, TRUE otherwise
 */
b8 perspective_matrix(f32 near, f32 far, f32 left, f32 right, f32 top, f32 bottom, mat4x4* outMatrix);

/**
 * @brief Creates a orthographic projection matrix
 * @param near The distance from the camera that the camera starts rendering
 * @param far The distance from the camera that the camera stops rendering
 * @param left The length from the center of the extent to the left edge
 * @param right The length from the center of the extent to the right edge
 * @param top The length from the center of the extent to the top edge
 * @param bottom The length from the center of the extent to the bottom edge
 * @param outMatrix A pointer to the matrix to store the output
 * @return FALSE if left = right, top = bottom or near = far, TRUE otherwise
 */
b8 orthographic_matrix(f32 near, f32 far, f32 left, f32 right, f32 top, f32 bottom, mat4x4* outMatrix);

/**
 * @brief creates a perspective projection matrix based on aspect ratio and field of view
 * @param nearClip The distance from the camera that the camera starts rendering
 * @param farClip The distance from the camera that the camera stops rendering
 * @param degreeView The field of view in degrees
 * @param aspectRatio The aspect ratio of the extent
 * @return A pass by reference 4x4 matrix containing the projection matrix
 */
mat4 perspective_projection_matrix(f32 nearClip, f32 farClip, f32 degreeView, f32 aspectRatio);

/**
 * @brief creates an orthographic projection matrix based on aspect ratio and field of view
 * @param nearClip The distance from the camera that the camera starts rendering
 * @param farClip The distance from the camera that the camera stops rendering
 * @param topDistance The vertical size of the projection from center to top
 * @param aspectRatio The aspect ratio of the extent
 * @return A pass by reference 4x4 matrix containing the projection matrix
 */
mat4 orthographic_projection_matrix(f32 nearClip, f32 farClip, f32 topDistance, f32 aspectRatio);

b8 decompose_model_matrix(mat4 matrix, vec3* out_translation, quat* out_quat, vec3* out_scale);

/**
 * @brief Turns the rotation in degrees around each access into a quaternion for the rotation matrix
 * @param eulerDegreeAngles The angle around the 3 axies in degrees
 * @return A pass by reference quaternion
 */
quat euler_to_quat(vec3 eulerDegreeAngles);

/*! 
 * @brief Clamps the value between the minimum and maximum
 * @param value An interger to be contained
 * @param minimum The minimum that value is allowed to be
 * @param maximum The maximum that value is allowed to be
 * @return value if the value is between minimum and maximum, minimum if value is smaller than minimum and likewise with maximum
 */
u64 vclamp(u64 value, u64 minimum, u64 maximum);