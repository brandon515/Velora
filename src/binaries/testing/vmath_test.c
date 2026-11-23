#include "vmath_test.h"
#include "test_framework.h"
#include "core/utils/vmath.h"

#include <math.h>

#define EPS 1e-4

/*
 * mat4 stores aRC (row R, column C) column by column: mat[c*4 + r]. The
 * shaders use matrices the same way (GLSL is column-major, translation in
 * a14/a24/a34). apply() does M*v by that definition, independently of the
 * library's own multiply functions, so these tests check what a matrix
 * actually does to a point.
 */
static vec4 apply(mat4 m, vec4 v) {
  vec4 out;
  for(int r = 0; r < 4; r++) {
    out.xyzw[r] = 0;
    for(int c = 0; c < 4; c++) {
      out.xyzw[r] += m.mat[c * 4 + r] * v.xyzw[c];
    }
  }
  return out;
}

static vec4 point(f32 x, f32 y, f32 z) {
  return (vec4){{x, y, z, 1}};
}

static b8 vec4_near(vec4 a, vec4 b, f64 eps) {
  for(int i = 0; i < 4; i++) {
    if(fabs(a.xyzw[i] - b.xyzw[i]) > eps) {
      return FALSE;
    }
  }
  return TRUE;
}

#define CHECK_VEC4(actual, expected, msg) \
  do { \
    vec4 _act = (actual); \
    vec4 _exp = (expected); \
    if(!vec4_near(_act, _exp, EPS)) { \
      VERROR("    got (%f, %f, %f, %f), expected (%f, %f, %f, %f)", _act.x, _act.y, _act.z, _act.w, _exp.x, _exp.y, _exp.z, _exp.w); \
    } \
    CHECK(vec4_near(_act, _exp, EPS), msg); \
  } while(0)

// Rotation of angle_degrees about a unit axis, as a quaternion (x, y, z, w).
static quat axis_angle(f32 ax, f32 ay, f32 az, f32 angle_degrees) {
  f32 half = angle_degrees * (f32)(3.14159265358979 / 180.0) / 2;
  f32 s = sinf(half);
  return (quat){{ax * s, ay * s, az * s, cosf(half)}};
}

// Quaternions q and -q are the same rotation, so a must equal b or -b component by component.
static b8 same_rotation(quat a, quat b) {
  b8 same = TRUE;
  b8 negated = TRUE;
  for(int i = 0; i < 4; i++) {
    same = same && fabs(a.xyzw[i] - b.xyzw[i]) < 1e-3;
    negated = negated && fabs(a.xyzw[i] + b.xyzw[i]) < 1e-3;
  }
  return same || negated;
}

static void test_identity(void) {
  mat4 id = indentity_mat4();
  CHECK_VEC4(apply(id, point(1, 2, 3)), point(1, 2, 3), "identity leaves a point alone");
  mat3 id3 = indentity_mat3();
  CHECK(id3.a11 == 1 && id3.a22 == 1 && id3.a33 == 1 && id3.a12 == 0 && id3.a31 == 0, "3x3 identity");
  mat2 id2 = indentity_mat2();
  CHECK(id2.a11 == 1 && id2.a22 == 1 && id2.a12 == 0 && id2.a21 == 0, "2x2 identity");
}

static void test_translation_matrix(void) {
  mat4 t = translation_matrix((vec3){{10, 20, 30}});
  CHECK_VEC4(apply(t, point(1, 2, 3)), point(11, 22, 33), "translation moves a point");
  vec4 dir = {{1, 2, 3, 0}};
  CHECK_VEC4(apply(t, dir), dir, "translation does not move a direction");
}

static void test_scaling_matrix(void) {
  mat4 s = scaling_matrix((vec3){{2, 3, 4}});
  CHECK_VEC4(apply(s, point(1, 1, 1)), point(2, 3, 4), "scaling scales each axis");
}

// bug-report #19: rotation_matrix is transposed, so quaternions rotate the wrong way.
static void test_rotation_matrix_z_90(void) {
  mat4 r = rotation_matrix(axis_angle(0, 0, 1, 90));
  CHECK_VEC4(apply(r, point(1, 0, 0)), point(0, 1, 0), "+90 degrees about Z takes X to Y");
  CHECK_VEC4(apply(r, point(0, 1, 0)), point(-1, 0, 0), "+90 degrees about Z takes Y to -X");
  CHECK_VEC4(apply(r, point(0, 0, 1)), point(0, 0, 1), "+90 degrees about Z leaves Z alone");
}

static void test_rotation_matrix_x_90(void) {
  mat4 r = rotation_matrix(axis_angle(1, 0, 0, 90));
  CHECK_VEC4(apply(r, point(0, 1, 0)), point(0, 0, 1), "+90 degrees about X takes Y to Z");
}

static void test_rotation_matrix_identity_quat(void) {
  mat4 r = rotation_matrix((quat){{0, 0, 0, 1}});
  CHECK_VEC4(apply(r, point(1, 2, 3)), point(1, 2, 3), "identity quaternion gives no rotation");
}

static void test_euler_to_quat(void) {
  quat q = euler_to_quat((vec3){{0, 0, 90}});
  CHECK(same_rotation(q, axis_angle(0, 0, 1, 90)), "euler (0, 0, 90) is +90 degrees about Z");
  q = euler_to_quat((vec3){{90, 0, 0}});
  CHECK(same_rotation(q, axis_angle(1, 0, 0, 90)), "euler (90, 0, 0) is +90 degrees about X");
  q = euler_to_quat((vec3){{0, 0, 0}});
  CHECK(same_rotation(q, (quat){{0, 0, 0, 1}}), "euler zero is the identity quaternion");
}

static void test_euler_round_trip(void) {
  vec3 angles = {{10, 20, 30}};
  vec3 back = quat_to_euler(euler_to_quat(angles));
  CHECK_NEAR(back.x, 10, 1e-2, "roll round-trips");
  CHECK_NEAR(back.y, 20, 1e-2, "pitch round-trips");
  CHECK_NEAR(back.z, 30, 1e-2, "yaw round-trips");
}

/*
 * Pins the library's multiply convention: matrix4_multiply(a, b) applies a
 * first and then b (mathematically b*a). model_matrix and the MVP code
 * depend on it, so a fix for bug #21 must not flip it by accident.
 */
static void test_matrix4_multiply_convention(void) {
  mat4 s = scaling_matrix((vec3){{2, 2, 2}});
  mat4 t = translation_matrix((vec3){{10, 0, 0}});
  CHECK_VEC4(apply(matrix4_multiply(s, t), point(1, 0, 0)), point(12, 0, 0), "multiply(scale, translate) scales then translates");
  CHECK_VEC4(apply(matrix4_multiply(t, s), point(1, 0, 0)), point(22, 0, 0), "multiply(translate, scale) translates then scales");
}

static void test_matrix_multiply_identity(void) {
  mat4 m = model_matrix((vec3){{1, 2, 3}}, (quat){{0, 0, 0, 1}}, (vec3){{2, 3, 4}});
  mat4 a = matrix4_multiply(m, indentity_mat4());
  mat4 b = matrix4_multiply(indentity_mat4(), m);
  b8 same = TRUE;
  for(int i = 0; i < 16; i++) {
    same = same && fabs(a.mat[i] - m.mat[i]) < EPS && fabs(b.mat[i] - m.mat[i]) < EPS;
  }
  CHECK(same, "multiplying by identity changes nothing");
}

// Not in the bug report: matrix_vec4_multiply computes M^T*v, so it ignores translation.
static void test_matrix_vec4_multiply(void) {
  mat4 t = translation_matrix((vec3){{10, 20, 30}});
  CHECK_VEC4(matrix_vec4_multiply(t, point(1, 2, 3)), point(11, 22, 33), "matrix_vec4_multiply applies a translation");
  mat4 s = scaling_matrix((vec3){{2, 3, 4}});
  CHECK_VEC4(matrix_vec4_multiply(s, point(1, 1, 1)), point(2, 3, 4), "matrix_vec4_multiply applies a scale");
}

static void test_model_matrix_without_rotation(void) {
  mat4 m = model_matrix((vec3){{1, 2, 3}}, (quat){{0, 0, 0, 1}}, (vec3){{2, 3, 4}});
  CHECK_VEC4(apply(m, point(1, 1, 1)), point(3, 5, 7), "model matrix scales then translates");
}

// Depends on bug-report #19 (rotation_matrix).
static void test_model_matrix_order(void) {
  mat4 m = model_matrix((vec3){{1, 0, 0}}, axis_angle(0, 0, 1, 90), (vec3){{2, 2, 2}});
  CHECK_VEC4(apply(m, point(1, 0, 0)), point(1, 2, 0), "model matrix scales, then rotates, then translates");
}

static void test_matrix4_invert(void) {
  mat4 m = model_matrix((vec3){{1, -2, 3}}, axis_angle(0, 1, 0, 30), (vec3){{2, 3, 4}});
  mat4 inv;
  REQUIRE(matrix4_invert(m, &inv) == TRUE, "invertible matrix inverts");
  vec4 p = point(5, -6, 7);
  CHECK_VEC4(apply(inv, apply(m, p)), p, "inverse undoes the matrix");
  CHECK_VEC4(apply(m, apply(inv, p)), p, "matrix undoes the inverse");

  mat4 singular = scaling_matrix((vec3){{1, 0, 1}});
  CHECK(matrix4_invert(singular, &inv) == FALSE, "singular matrix does not invert");
}

static void test_decompose_round_trip(void) {
  vec3 t = {{1, 2, 3}};
  quat q = vec4_normalize(euler_to_quat((vec3){{10, 20, 30}}));
  vec3 s = {{2, 3, 4}};
  vec3 out_t, out_s;
  quat out_q;
  decompose_model_matrix(model_matrix(t, q, s), &out_t, &out_q, &out_s);
  CHECK_VEC4(((vec4){{out_t.x, out_t.y, out_t.z, 1}}), point(1, 2, 3), "translation round-trips");
  CHECK_VEC4(((vec4){{out_s.x, out_s.y, out_s.z, 1}}), point(2, 3, 4), "scale round-trips");
  CHECK(same_rotation(out_q, q), "rotation round-trips");
}

/*
 * bug-report #20: rotation_to_quat's fallback branches (taken when w is near
 * 0, i.e. rotations near 180 degrees) add where they should subtract for w.
 */
static void test_decompose_180_degree_rotation(void) {
  quat q = {{2.0f / 7, 3.0f / 7, 6.0f / 7, 0}}; // 180 degrees about (2, 3, 6)/7
  vec3 out_t, out_s;
  quat out_q;
  decompose_model_matrix(model_matrix((vec3){{0, 0, 0}}, q, (vec3){{1, 1, 1}}), &out_t, &out_q, &out_s);
  if(!same_rotation(out_q, q)) {
    VERROR("    got (%f, %f, %f, %f), expected +/-(%f, %f, %f, %f)", out_q.x, out_q.y, out_q.z, out_q.w, q.x, q.y, q.z, q.w);
  }
  CHECK(same_rotation(out_q, q), "180 degree rotation round-trips through decompose");
}

/*
 * Tier 2: the projection matrices map depth to OpenGL's [-1, 1], but Vulkan
 * expects the near plane at 0 and the far plane at 1.
 */
static f32 ndc_depth(mat4 proj, f32 view_z) {
  vec4 clip = apply(proj, point(0, 0, view_z));
  return clip.z / clip.w;
}

static void test_perspective_depth_range(void) {
  mat4 p = perspective_projection_matrix(0.1f, 100.0f, 60.0f, 16.0f / 9.0f);
  CHECK_NEAR(ndc_depth(p, -0.1f), 0.0, 1e-3, "near plane maps to depth 0");
  CHECK_NEAR(ndc_depth(p, -100.0f), 1.0, 1e-3, "far plane maps to depth 1");
}

static void test_orthographic_depth_range(void) {
  mat4 p = orthographic_projection_matrix(0.1f, 100.0f, 10.0f, 16.0f / 9.0f);
  CHECK_NEAR(ndc_depth(p, -0.1f), 0.0, 1e-3, "near plane maps to depth 0");
  CHECK_NEAR(ndc_depth(p, -100.0f), 1.0, 1e-3, "far plane maps to depth 1");
}

static void test_perspective_xy(void) {
  // 90 degree FOV and aspect 1: a point at 45 degrees lands on the edge of the screen.
  mat4 p = perspective_projection_matrix(1.0f, 100.0f, 90.0f, 1.0f);
  vec4 clip = apply(p, point(5, 0, -5));
  CHECK_NEAR(fabs(clip.x / clip.w), 1.0, 1e-3, "45 degree point is on the edge of a 90 degree frustum");
  clip = apply(p, point(0, 0, -5));
  CHECK_NEAR(clip.x / clip.w, 0.0, 1e-5, "point on the view axis is centred");
}

static void test_projection_rejects_degenerate(void) {
  mat4 out;
  CHECK(perspective_matrix(1, 1, -1, 1, 1, -1, &out) == FALSE, "near == far rejected");
  CHECK(perspective_matrix(1, 10, 1, 1, 1, -1, &out) == FALSE, "left == right rejected");
  CHECK(orthographic_matrix(1, 10, -1, 1, 1, 1, &out) == FALSE, "top == bottom rejected");
  CHECK(perspective_matrix(1, 10, -1, 1, 1, -1, &out) == TRUE, "valid frustum accepted");
}

static void test_vector_ops(void) {
  vec3 n = vec3_normalize((vec3){{3, 0, 4}});
  CHECK_NEAR(n.x, 0.6, EPS, "normalize x");
  CHECK_NEAR(n.z, 0.8, EPS, "normalize z");
  vec2 n2 = vec2_normalize((vec2){{0, 5}});
  CHECK_NEAR(n2.y, 1.0, EPS, "normalize vec2");
  vec4 n4 = vec4_normalize((vec4){{1, 1, 1, 1}});
  CHECK_NEAR(n4.w, 0.5, EPS, "normalize vec4");
  vec3 sum = vec3_add((vec3){{1, 2, 3}}, (vec3){{4, 5, 6}});
  CHECK(sum.x == 5 && sum.y == 7 && sum.z == 9, "vec3_add");
  vec2 sum2 = vec2_add((vec2){{1, 2}}, (vec2){{3, 4}});
  CHECK(sum2.x == 4 && sum2.y == 6, "vec2_add");
  vec3 scaled = vec3_multiply_scalar((vec3){{1, -2, 3}}, 2);
  CHECK(scaled.x == 2 && scaled.y == -4 && scaled.z == 6, "vec3_multiply_scalar");
  vec4 scaled4 = vec4_multiply_scalar((vec4){{1, 2, 3, 4}}, 0.5f);
  CHECK(scaled4.x == 0.5f && scaled4.w == 2, "vec4_multiply_scalar");
}

static void test_integer_helpers(void) {
  CHECK(vclamp(5, 1, 10) == 5, "clamp inside range");
  CHECK(vclamp(0, 1, 10) == 1, "clamp below range");
  CHECK(vclamp(11, 1, 10) == 10, "clamp above range");
  CHECK(round_up_to_largest_power_of_two(1) == 1, "round 1");
  CHECK(round_up_to_largest_power_of_two(5) == 8, "round 5 up to 8");
  CHECK(round_up_to_largest_power_of_two(8) == 8, "power of two stays");
  CHECK(round_up_to_largest_power_of_two(1025) == 2048, "round 1025 up to 2048");
  CHECK(log_base_2(8) == 3, "log2(8)");
  CHECK(log_base_2(5) == 3, "log2 rounds up");
  CHECK(log_base_2(1) == 0, "log2(1)");
  CHECK(log_base_2_unchecked(1ULL << 40) == 40, "log2(2^40)");
  CHECK(log_base_2_unchecked(1ULL << 63) == 63, "log2(2^63)");
}

void vmath_test(void) {
  RUN_TEST(test_identity);
  RUN_TEST(test_translation_matrix);
  RUN_TEST(test_scaling_matrix);
  RUN_TEST(test_rotation_matrix_z_90);
  RUN_TEST(test_rotation_matrix_x_90);
  RUN_TEST(test_rotation_matrix_identity_quat);
  RUN_TEST(test_euler_to_quat);
  RUN_TEST(test_euler_round_trip);
  RUN_TEST(test_matrix4_multiply_convention);
  RUN_TEST(test_matrix_multiply_identity);
  RUN_TEST(test_matrix_vec4_multiply);
  RUN_TEST(test_model_matrix_without_rotation);
  RUN_TEST(test_model_matrix_order);
  RUN_TEST(test_matrix4_invert);
  RUN_TEST(test_decompose_round_trip);
  RUN_TEST(test_decompose_180_degree_rotation);
  RUN_TEST(test_perspective_depth_range);
  RUN_TEST(test_orthographic_depth_range);
  RUN_TEST(test_perspective_xy);
  RUN_TEST(test_projection_rejects_degenerate);
  RUN_TEST(test_vector_ops);
  RUN_TEST(test_integer_helpers);
}
