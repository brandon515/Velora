#include "ecs_test.h"
#include "test_framework.h"
#include "ecs/vecs.h"
#include "ecs/vtransform.h"

#include <math.h>

/*
 * Every test initialises and shuts down the ECS itself, so RUN_TEST's leak
 * check covers the whole lifecycle.
 */

static b8 attach_u64(vcomponent_type type, u64 entity, u64 value) {
  return attach_component(type, entity, sizeof(value), &value);
}

static u64 read_u64(u64 entity, vcomponent_type type) {
  u64* p = get_component_data(entity, type);
  return p == NULL ? U64_MAX : *p;
}

static void test_init_and_shutdown(void) {
  CHECK(shutdown_entity_component_system() == FALSE, "shutdown before init returns FALSE");
  CHECK(initilize_entity_component_system() == TRUE, "first init returns TRUE");
  CHECK(initilize_entity_component_system() == FALSE, "second init returns FALSE");
  CHECK(shutdown_entity_component_system() == TRUE, "shutdown returns TRUE");
  CHECK(initilize_entity_component_system() == TRUE, "init works again after shutdown");
  CHECK(shutdown_entity_component_system() == TRUE, "second shutdown returns TRUE");
}

static void test_calls_before_init_fail(void) {
  u64 v = 1;
  CHECK(attach_component(VELORA_COMPONENT_TRANSFORM, 0, sizeof(v), &v) == FALSE, "attach before init fails");
  CHECK(get_component_data(0, VELORA_COMPONENT_TRANSFORM) == NULL, "get before init returns NULL");
  CHECK(delete_component(VELORA_COMPONENT_TRANSFORM, 0) == FALSE, "delete_component before init fails");
  CHECK(delete_entity(0) == FALSE, "delete_entity before init fails");
}

static void test_attach_and_get(void) {
  initilize_entity_component_system();
  CHECK(attach_u64(VELORA_COMPONENT_TRANSFORM, 3, 30) == TRUE, "attach a component");
  CHECK(read_u64(3, VELORA_COMPONENT_TRANSFORM) == 30, "get the attached data back");
  CHECK(get_component_data(4, VELORA_COMPONENT_TRANSFORM) == NULL, "entity without the component gives NULL");
  CHECK(get_component_data(3, VELORA_COMPONENT_CAMERA) == NULL, "unregistered component type gives NULL");
  shutdown_entity_component_system();
}

static void test_attach_duplicate_fails(void) {
  initilize_entity_component_system();
  attach_u64(VELORA_COMPONENT_TRANSFORM, 1, 10);
  attach_u64(VELORA_COMPONENT_TRANSFORM, 2, 20);
  CHECK(attach_u64(VELORA_COMPONENT_TRANSFORM, 1, 99) == FALSE, "second component of the same type is rejected");
  CHECK(read_u64(1, VELORA_COMPONENT_TRANSFORM) == 10, "original data kept");
  shutdown_entity_component_system();
}

static void test_attach_rejects_out_of_range_entity(void) {
  initilize_entity_component_system();
  CHECK(attach_u64(VELORA_COMPONENT_TRANSFORM, VELORA_MAX_OBJECTS, 1) == FALSE, "entity id at VELORA_MAX_OBJECTS rejected");
  shutdown_entity_component_system();
}

/*
 * Regression: attaching a component for an entity whose id is lower than
 * one already in the list goes through darray_insert.
 */
static void test_attach_out_of_order_ids(void) {
  initilize_entity_component_system();
  u64 ids[] = {50, 10, 30, 5, 40, 20, 0, 60};
  for(u64 i = 0; i < 8; i++) {
    CHECK(attach_u64(VELORA_COMPONENT_TRANSFORM, ids[i], ids[i] * 100) == TRUE, "attach out of order");
  }
  b8 all_found = TRUE;
  for(u64 i = 0; i < 8; i++) {
    all_found = all_found && read_u64(ids[i], VELORA_COMPONENT_TRANSFORM) == ids[i] * 100;
  }
  CHECK(all_found, "every entity gets its own data back");

  darray* list = NULL;
  REQUIRE(get_components(VELORA_COMPONENT_TRANSFORM, &list) == TRUE, "get_components finds the list");
  CHECK(list->length == 8, "list holds 8 components");
  b8 sorted = TRUE;
  vcomponent* comps = list->data;
  for(u64 i = 1; i < list->length; i++) {
    sorted = sorted && comps[i - 1].entityID < comps[i].entityID;
  }
  CHECK(sorted, "component list is sorted by entity id");
  shutdown_entity_component_system();
}

static void test_get_components_unknown_type(void) {
  initilize_entity_component_system();
  darray* list = NULL;
  CHECK(get_components(VELORA_COMPONENT_CAMERA, &list) == FALSE, "get_components for an unused type returns FALSE");
  shutdown_entity_component_system();
}

static void test_create_new_entity(void) {
  initilize_entity_component_system();
  u64 a = create_new_entity();
  CHECK(a == 0, "first entity is 0");
  CHECK(create_new_entity() == a, "same id until a component is attached");
  attach_u64(VELORA_COMPONENT_TRANSFORM, a, 1);
  u64 b = create_new_entity();
  CHECK(b == 1, "next entity is 1");
  attach_u64(VELORA_COMPONENT_TRANSFORM, b, 1);
  delete_entity(a);
  CHECK(create_new_entity() == 0, "deleted entity id is reused");
  shutdown_entity_component_system();
}

// Regression: delete_component passed the element buffer instead of the darray.
static void test_delete_component(void) {
  initilize_entity_component_system();
  for(u64 e = 0; e < 5; e++) {
    attach_u64(VELORA_COMPONENT_TRANSFORM, e, e + 100);
  }
  CHECK(delete_component(VELORA_COMPONENT_TRANSFORM, 2) == TRUE, "delete an attached component");
  CHECK(get_component_data(2, VELORA_COMPONENT_TRANSFORM) == NULL, "deleted component is gone");
  b8 others_ok = TRUE;
  for(u64 e = 0; e < 5; e++) {
    if(e != 2) {
      others_ok = others_ok && read_u64(e, VELORA_COMPONENT_TRANSFORM) == e + 100;
    }
  }
  CHECK(others_ok, "other components untouched");
  CHECK(delete_component(VELORA_COMPONENT_TRANSFORM, 2) == FALSE, "deleting it again returns FALSE");
  CHECK(delete_component(VELORA_COMPONENT_CAMERA, 1) == FALSE, "deleting an unused type returns FALSE");
  darray* list = NULL;
  get_components(VELORA_COMPONENT_TRANSFORM, &list);
  CHECK(list != NULL && list->length == 4, "list shrank by one");
  shutdown_entity_component_system();
}

// Regression: delete_entity used i where it meant j.
static void test_delete_entity(void) {
  initilize_entity_component_system();
  for(u64 e = 0; e < 4; e++) {
    attach_u64(VELORA_COMPONENT_TRANSFORM, e, e + 100);
    attach_u64(VELORA_COMPONENT_RENDERABLE, e, e + 200);
  }
  attach_u64(VELORA_COMPONENT_CAMERA, 2, 302);
  CHECK(delete_entity(2) == TRUE, "delete an entity with components");
  CHECK(get_component_data(2, VELORA_COMPONENT_TRANSFORM) == NULL, "its transform is gone");
  CHECK(get_component_data(2, VELORA_COMPONENT_RENDERABLE) == NULL, "its renderable is gone");
  CHECK(get_component_data(2, VELORA_COMPONENT_CAMERA) == NULL, "its camera is gone");
  b8 others_ok = TRUE;
  for(u64 e = 0; e < 4; e++) {
    if(e != 2) {
      others_ok = others_ok && read_u64(e, VELORA_COMPONENT_TRANSFORM) == e + 100 && read_u64(e, VELORA_COMPONENT_RENDERABLE) == e + 200;
    }
  }
  CHECK(others_ok, "other entities untouched");
  CHECK(delete_entity(2) == FALSE, "deleting it again returns FALSE");
  CHECK(delete_entity(7) == FALSE, "deleting an entity with no components returns FALSE");
  shutdown_entity_component_system();
}

static void test_delete_component_then_entity(void) {
  initilize_entity_component_system();
  attach_u64(VELORA_COMPONENT_TRANSFORM, 1, 1);
  attach_u64(VELORA_COMPONENT_RENDERABLE, 1, 2);
  delete_component(VELORA_COMPONENT_TRANSFORM, 1);
  CHECK(delete_entity(1) == TRUE, "entity with one component left can be deleted");
  CHECK(get_component_data(1, VELORA_COMPONENT_RENDERABLE) == NULL, "remaining component removed");
  delete_component(VELORA_COMPONENT_RENDERABLE, 3);
  CHECK(delete_entity(1) == FALSE, "entity has nothing left");
  shutdown_entity_component_system();
}

// Issue #24: delete_entity indexes entities[] before checking the id is in range.
static void test_delete_entity_out_of_range(void) {
  initilize_entity_component_system();
  CHECK(delete_entity(VELORA_MAX_OBJECTS) == FALSE, "delete_entity rejects an id past VELORA_MAX_OBJECTS");
  CHECK(delete_entity(U64_MAX) == FALSE, "delete_entity rejects U64_MAX");
  shutdown_entity_component_system();
}

static void test_shutdown_frees_components(void) {
  initilize_entity_component_system();
  for(u64 e = 0; e < 20; e++) {
    attach_u64(VELORA_COMPONENT_TRANSFORM, e, e);
    if(e % 2 == 0) {
      attach_u64(VELORA_COMPONENT_RENDERABLE, e, e);
    }
  }
  shutdown_entity_component_system();
  // RUN_TEST's leak check verifies everything was freed.
}

// --- transforms ---

static vec3 transform_point(mat4 m, vec3 p) {
  vec3 out;
  for(int r = 0; r < 3; r++) {
    out.xyz[r] = m.mat[0 * 4 + r] * p.x + m.mat[1 * 4 + r] * p.y + m.mat[2 * 4 + r] * p.z + m.mat[3 * 4 + r];
  }
  return out;
}

static b8 vec3_near(vec3 a, f32 x, f32 y, f32 z) {
  return fabs(a.x - x) < 1e-4 && fabs(a.y - y) < 1e-4 && fabs(a.z - z) < 1e-4;
}

static void test_register_and_edit_transform(void) {
  initilize_entity_component_system();
  CHECK(register_transform(0, 1, 2, 3, 0, 0, 0, 1, 1, 1, NULL) == TRUE, "register a transform");
  CHECK(register_empty_transform(0, NULL) == FALSE, "second transform on the same entity rejected");
  vtransform* t = get_transform_component(0);
  REQUIRE(t != NULL, "transform component found");
  CHECK(t->position.x == 1 && t->position.y == 2 && t->position.z == 3, "position stored");
  transform_move(t, 1, 1, 1);
  CHECK(t->position.x == 2 && t->position.y == 3 && t->position.z == 4, "transform_move adds");
  transform_set_position(t, 0, 0, 0);
  transform_set_scale(t, 2, 2, 2);
  transform_set_rotation(t, 0, 0, 45);
  transform_rotate(t, (vec3){{0, 0, 2}}, 45);
  CHECK(fabs(t->rotation.z - 90) < 1e-4, "transform_rotate adds degrees about the normalised axis");
  CHECK(get_transform_component(1) == NULL, "entity without a transform gives NULL");
  shutdown_entity_component_system();
}

static void test_model_matrix_no_parent(void) {
  vtransform t = create_transform(1, 2, 3, 0, 0, 0, 2, 2, 2, NULL);
  vec3 p = transform_point(transform_get_model_matrix(&t), (vec3){{1, 1, 1}});
  CHECK(vec3_near(p, 3, 4, 5), "model matrix scales then translates");
}

/*
 * Issue #19: transform_get_model_matrix combines parent and child in
 * the wrong order. A child 1 unit along X under a parent scaled by 2 should
 * sit 2 units along X in world space; local*parent puts it at 1.
 */
static void test_model_matrix_with_parent(void) {
  vtransform parent = create_transform(0, 0, 0, 0, 0, 0, 2, 2, 2, NULL);
  vtransform child = create_transform(1, 0, 0, 0, 0, 0, 1, 1, 1, &parent);
  vec3 p = transform_point(transform_get_model_matrix(&child), (vec3){{0, 0, 0}});
  if(!vec3_near(p, 2, 0, 0)) {
    VERROR("    child origin at (%f, %f, %f), expected (2, 0, 0)", p.x, p.y, p.z);
  }
  CHECK(vec3_near(p, 2, 0, 0), "parent scale applies to the child's position");

  vtransform moved_parent = create_transform(10, 0, 0, 0, 0, 0, 1, 1, 1, NULL);
  vtransform scaled_child = create_transform(0, 0, 0, 0, 0, 0, 3, 3, 3, &moved_parent);
  p = transform_point(transform_get_model_matrix(&scaled_child), (vec3){{1, 0, 0}});
  if(!vec3_near(p, 13, 0, 0)) {
    VERROR("    child point at (%f, %f, %f), expected (13, 0, 0)", p.x, p.y, p.z);
  }
  CHECK(vec3_near(p, 13, 0, 0), "parent translation is not scaled by the child");
}

void ecs_test(void) {
  RUN_TEST(test_init_and_shutdown);
  RUN_TEST(test_calls_before_init_fail);
  RUN_TEST(test_attach_and_get);
  RUN_TEST(test_attach_duplicate_fails);
  RUN_TEST(test_attach_rejects_out_of_range_entity);
  RUN_TEST(test_attach_out_of_order_ids);
  RUN_TEST(test_get_components_unknown_type);
  RUN_TEST(test_create_new_entity);
  RUN_TEST(test_delete_component);
  RUN_TEST(test_delete_entity);
  RUN_TEST(test_delete_component_then_entity);
  RUN_TEST(test_shutdown_frees_components);
  RUN_TEST(test_register_and_edit_transform);
  RUN_TEST(test_model_matrix_no_parent);
  RUN_TEST(test_model_matrix_with_parent);
  RUN_TEST(test_delete_entity_out_of_range);
}
