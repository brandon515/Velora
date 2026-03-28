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

/**
 * @brief Normalizes the vector provided
 * @param vector The vector to be normalized
 * @return The normalized vector
 */
vec2 vec2_normalize(vec2 vector);

/**
 * @brief Normalizes the vector provided
 * @param vector The vector to be normalized
 * @return The normalized vector
 */
vec3 vec3_normalize(vec3 vector);

/**
 * @brief Normalizes the vector provided
 * @param vector The vector to be normalized
 * @return The normalized vector
 */
vec4 vec4_normalize(vec4 vector);

/**
 * @brief Scale the vector by the scalar provided
 * @param vector The vector to be scaled
 * @param scalar The scalar value to scale the entire vector
 * @return The scaled up vector
 */
vec2 vec2_multiply_scalar(vec2 vector, f32 scalar);

/**
 * @brief Scale the vector by the scalar provided
 * @param vector The vector to be scaled
 * @param scalar The scalar value to scale the entire vector
 * @return The scaled up vector
 */
vec3 vec3_multiply_scalar(vec3 vector, f32 scalar);

/**
 * @brief Scale the vector by the scalar provided
 * @param vector The vector to be scaled
 * @param scalar The scalar value to scale the entire vector
 * @return The scaled up vector
 */
vec4 vec4_multiply_scalar(vec4 vector, f32 scalar);

/**
 * @brief Add the two vectors together
 * @param vector1 The first vector
 * @param vector2 The second vector
 * @return A new vector with all the members of the vectors of vector1 and vector2 added together
 */
vec2 vec2_add(vec2 vector1, vec2 vector2);

/**
 * @brief Add the two vectors together
 * @param vector1 The first vector
 * @param vector2 The second vector
 * @return A new vector with all the members of the vectors of vector1 and vector2 added together
 */
vec3 vec3_add(vec3 vector1, vec3 vector2);

/**
 * @brief Add the two vectors together
 * @param vector1 The first vector
 * @param vector2 The second vector
 * @return A new vector with all the members of the vectors of vector1 and vector2 added together
 */
vec4 vec4_add(vec4 vector1, vec4 vector2);

/**
 * @brief Gets a translation matrix based on the translation vector
 * @param translation A vector containing a translation of the object from the origin
 * @return A 4x4 matrix that is the translation matrix
 */
mat4 translation_matrix(vec3 translation);

/**
 * @brief Gets a scaling matrix based on the scaling of the individual axies
 * @param scale A vector containing the scale of the object on each axis
 * @return A 4x4 matrix that is the scaling matrix
 */
mat4 scaling_matrix(vec3 scale);

/**
 * @brief Gets a rotation matrix based on the quaternion
 * @param quaternion A quaternion that represents the rotation of the oject
 * @return A 4x4 matrix that is the rotation matrix
 */
mat4 rotation_matrix(quat quaternion);

/**
 * @brief A helper function that returns an identity matrix
 * @return A 2x2 identity matrix
 */
mat2 indentity_mat2();

/**
 * @brief A helper function that returns an identity matrix
 * @return A 3x3 identity matrix
 */
mat3 indentity_mat3();

/**
 * @brief A helper function that returns an identity matrix
 * @return A 4x4 identity matrix
 */
mat4 indentity_mat4();

/**
 * @brief Type casting for multiplying a 2x2 matrix with a 4 member vector
 * @param mat A 2x2 matrix passed by reference
 * @param vec A 2 member vector passed by reference
 * @return A vector with the results
 */
vec2 matrix_vec2_multiply(mat2x2 mat, vec2 vec);

/**
 * @brief Type casting for multiplying a 2x2 matrix with another
 * @param mat1 A 2x2 matrix passed by reference
 * @param mat2 A 2x2 matrix passed by reference
 * @return The result of the multiplied matrices
 */
mat2x2 matrix2_multiply(mat2x2 mat1, mat2x2 mat2);

/**
 * @brief Type casting for multiplying a 3x3 matrix with a 4 member vector
 * @param mat A 3x3 matrix passed by reference
 * @param vec A 3 member vector passed by reference
 * @return A vector with the results
 */
vec3 matrix_vec3_multiply(mat3x3 mat, vec3 vec);

/**
 * @brief Type casting for multiplying a 3x3 matrix with another
 * @param mat1 A 3x3 matrix passed by reference
 * @param mat2 A 3x3 matrix passed by reference
 * @return The result of the multiplied matrices
 */
mat3x3 matrix3_multiply(mat3x3 mat1, mat3x3 mat2);

/**
 * @brief Type casting for multiplying a 4x4 matrix with a 4 member vector
 * @param mat A 4x4 matrix passed by reference
 * @param vec A 4 member vector passed by reference
 * @return A vector with the results
 */
vec4 matrix_vec4_multiply(mat4x4 mat, vec4 vec);

/**
 * @brief Type casting for multiplying a 4x4 matrix with another
 * @param mat1 A 4x4 matrix passed by reference
 * @param mat2 A 4x4 matrix passed by reference
 * @return The result of the multiplied matrices
 */
mat4x4 matrix4_multiply(mat4x4 mat1, mat4x4 mat2);

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

/**
 * @brief creates an orthographic projection matrix based on aspect ratio and field of view
 * @param matrix The model matrix to decompose
 * @param out_translation A pointer to an allocated vec3 to put the translation vec3 data into
 * @param out_quat A pointer to an allocated quat to put the rotation data into
 * @param out_scale A pointer to an allocated vec3 to put the scale data into
 */
void decompose_model_matrix(mat4 matrix, vec3* out_translation, quat* out_quat, vec3* out_scale);

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