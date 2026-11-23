#include "vgltf_test.h"
#include "test_framework.h"
#include "assets/vgltf.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * Accessor tests build a gltf_object in memory, so they exercise
 * gltf_get_stream and the typed getters without file I/O. Import tests
 * write a small .gltf/.bin pair to the working directory (names start with
 * vt_gltf_ so they can't clash with other suites running in parallel).
 */

// --- in-memory accessor tests ---

typedef struct {
  gltf_object gltf;
  gltf_buffer buffer;
  gltf_buffer_view views[2];
  gltf_accessor accessors[2];
} fake_gltf;

// One buffer, one view covering all of it, one accessor at offset 0.
static void fake_init(fake_gltf* f, void* bytes, u64 size, u64 componentType, const char* type, u64 count) {
  *f = (fake_gltf){0};
  f->buffer.buffer = bytes;
  f->buffer.size = size;
  f->views[0].bufferIndex = 0;
  f->views[0].size = size;
  f->accessors[0].bufferViewIndex = 0;
  f->accessors[0].componentType = componentType;
  f->accessors[0].type = (char*)type;
  f->accessors[0].count = count;
  f->accessors[1] = f->accessors[0];
  f->gltf.buffers = &f->buffer;
  f->gltf.bufferCount = 1;
  f->gltf.bufferViews = f->views;
  f->gltf.bufferViewCount = 1;
  f->gltf.accessors = f->accessors;
  f->gltf.accessorCount = 1;
}

static void test_packed_vec3(void) {
  f32 data[6] = {1, 2, 3, 4, 5, 6};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 2);
  vec3 out[2] = {0};
  REQUIRE(gltf_accessor_get_vec3_array(&f.gltf, 0, out) == TRUE, "packed VEC3 accessor reads");
  CHECK(out[0].x == 1 && out[0].z == 3 && out[1].x == 4 && out[1].z == 6, "VEC3 values");
  u64 count = 0;
  CHECK(gltf_accessor_get_element_count(&f.gltf, 0, &count) && count == 2, "element count");
}

static void test_buffer_view_offset(void) {
  f32 data[4] = {-1, 7, 8, 9};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 1);
  f.views[0].offset = sizeof(f32);
  f.views[0].size = 3 * sizeof(f32);
  vec3 out = {0};
  REQUIRE(gltf_accessor_get_vec3_array(&f.gltf, 0, &out) == TRUE, "view with an offset reads");
  CHECK(out.x == 7 && out.y == 8 && out.z == 9, "view offset honoured");
}

// bug-report #17: the accessor's own byteOffset is ignored.
static void test_accessor_byte_offset(void) {
  f32 data[6] = {1, 2, 3, 4, 5, 6};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 1);
  f.accessors[0].offset = 3 * sizeof(f32);
  vec3 out = {0};
  REQUIRE(gltf_accessor_get_vec3_array(&f.gltf, 0, &out) == TRUE, "accessor with byteOffset reads");
  CHECK(out.x == 4 && out.y == 5 && out.z == 6, "accessor byteOffset honoured");
}

// bug-report #17: interleaved position/normal attributes sharing one strided view.
static void test_interleaved_attributes(void) {
  f32 data[12] = {
    1, 2, 3,  10, 20, 30,  // vertex 0: position, normal
    4, 5, 6,  40, 50, 60,  // vertex 1: position, normal
  };
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 2);
  f.views[0].stride = 6 * sizeof(f32);
  f.accessors[1].offset = 3 * sizeof(f32);
  f.gltf.accessorCount = 2;
  vec3 pos[2] = {0};
  vec3 nrm[2] = {0};
  REQUIRE(gltf_accessor_get_vec3_array(&f.gltf, 0, pos) == TRUE, "position accessor reads");
  CHECK(pos[0].x == 1 && pos[1].x == 4, "positions read with the view's stride");
  REQUIRE(gltf_accessor_get_vec3_array(&f.gltf, 1, nrm) == TRUE, "normal accessor reads");
  CHECK(nrm[0].x == 10 && nrm[0].z == 30 && nrm[1].x == 40 && nrm[1].z == 60, "normals read from their byteOffset");
}

// bug-report #18: reads are checked against the buffer, not the buffer view.
static void test_accessor_past_view_rejected(void) {
  f32 data[6] = {1, 2, 3, 4, 5, 6};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 2);
  f.views[0].size = 3 * sizeof(f32); // view only holds one VEC3
  vec3 out[2] = {0};
  CHECK(gltf_accessor_get_vec3_array(&f.gltf, 0, out) == FALSE, "accessor reading past its view's byteLength is rejected");
}

static void test_accessor_past_buffer_rejected(void) {
  f32 data[3] = {1, 2, 3};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 2);
  vec3 out[2] = {0};
  CHECK(gltf_accessor_get_vec3_array(&f.gltf, 0, out) == FALSE, "accessor reading past its buffer is rejected");
}

static void test_bad_indices_rejected(void) {
  f32 data[3] = {1, 2, 3};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC3", 1);
  gltf_data_stream stream = {0};
  CHECK(gltf_get_stream(&f.gltf, 1, &stream) == FALSE, "accessor index out of range");
  f.accessors[0].bufferViewIndex = 5;
  CHECK(gltf_get_stream(&f.gltf, 0, &stream) == FALSE, "buffer view index out of range");
  f.accessors[0].bufferViewIndex = 0;
  f.views[0].bufferIndex = 3;
  CHECK(gltf_get_stream(&f.gltf, 0, &stream) == FALSE, "buffer index out of range");
  u64 count = 0;
  CHECK(gltf_accessor_get_element_count(&f.gltf, 9, &count) == FALSE, "element count for a bad index");
}

static void test_wrong_type_rejected(void) {
  f32 data[4] = {1, 2, 3, 4};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_FLOAT, "VEC4", 1);
  vec3 out3[2];
  CHECK(gltf_accessor_get_vec3_array(&f.gltf, 0, out3) == FALSE, "VEC4 accessor is not read as VEC3");
  vec4 out4 = {0};
  CHECK(gltf_accessor_get_vec4_array(&f.gltf, 0, &out4) == TRUE && out4.w == 4, "VEC4 accessor reads as VEC4");
}

static void test_index_arrays_widen_to_u32(void) {
  u16 idx16[4] = {0, 1, 2, 65535};
  fake_gltf f;
  fake_init(&f, idx16, sizeof(idx16), GLTF_U16, "SCALAR", 4);
  u32 out[4] = {0};
  REQUIRE(gltf_accessor_get_u32_array(&f.gltf, 0, out) == TRUE, "u16 indices read as u32");
  CHECK(out[1] == 1 && out[3] == 65535, "u16 values widened");

  u8 idx8[3] = {3, 4, 255};
  fake_init(&f, idx8, sizeof(idx8), GLTF_U8, "SCALAR", 3);
  REQUIRE(gltf_accessor_get_u32_array(&f.gltf, 0, out) == TRUE, "u8 indices read as u32");
  CHECK(out[0] == 3 && out[2] == 255, "u8 values widened");

  u32 idx32[2] = {7, 100000};
  fake_init(&f, idx32, sizeof(idx32), GLTF_U32, "SCALAR", 2);
  REQUIRE(gltf_accessor_get_u32_array(&f.gltf, 0, out) == TRUE, "u32 indices read");
  CHECK(out[0] == 7 && out[1] == 100000, "u32 values");

  f32 floats[1] = {1};
  fake_init(&f, floats, sizeof(floats), GLTF_FLOAT, "SCALAR", 1);
  CHECK(gltf_accessor_get_u32_array(&f.gltf, 0, out) == FALSE, "float accessor is not read as indices");
}

// Tier 2: vector accessors that aren't floats (e.g. JOINTS_0 as unsigned byte VEC4) are rejected.
static void test_non_float_vector_accessor(void) {
  u8 joints[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  fake_gltf f;
  fake_init(&f, joints, sizeof(joints), GLTF_U8, "VEC4", 2);
  gltf_data_stream stream = {0};
  b8 ok = gltf_get_stream(&f.gltf, 0, &stream);
  CHECK(ok == TRUE, "unsigned byte VEC4 accessor (JOINTS_0) can be read");
  if(ok) {
    CHECK(stream.dataSize == 4 && stream.count == 2, "4 bytes per element");
    CHECK(stream.data[4] == 4 && stream.data[7] == 7, "joint indices read back");
    gltf_free_stream(stream);
  }
}

// --- file import tests ---

#define BIN_PATH "vt_gltf_data.bin"
#define GLTF_PATH "vt_gltf_model.gltf"

static const f32 k_positions[6] = {1, 2, 3, 4, 5, 6};

static b8 write_model(const char* gltf_json, const void* bin, u64 bin_size) {
  return write_test_file(BIN_PATH, bin, bin_size) && write_test_text_file(GLTF_PATH, gltf_json);
}

static void remove_model(void) {
  remove_test_file(BIN_PATH);
  remove_test_file(GLTF_PATH);
}

static const char* k_basic_gltf =
  "{\"asset\": {\"version\": \"2.0\", \"generator\": \"velora tests\"},"
  " \"scene\": 0,"
  " \"scenes\": [{\"nodes\": [0, 1]}],"
  " \"nodes\": ["
  "   {\"name\": \"root\", \"translation\": [1.5, 2.5, 3.5], \"scale\": [2.0, 2.0, 2.0], \"children\": [1]},"
  "   {\"rotation\": [0.0, 0.0, 0.7071068, 0.7071068]}"
  " ],"
  " \"buffers\": [{\"uri\": \"" BIN_PATH "\", \"byteLength\": 24}],"
  " \"bufferViews\": [{\"buffer\": 0, \"byteLength\": 24, \"target\": 34962}],"
  " \"accessors\": [{\"bufferView\": 0, \"componentType\": 5126, \"count\": 2, \"type\": \"VEC3\","
  "                  \"max\": [4.0, 5.0, 6.0], \"min\": [1.0, 2.0, 3.0]}]"
  "}";

static void test_import_basic_model(void) {
  REQUIRE(write_model(k_basic_gltf, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  REQUIRE(ok == TRUE, "basic model imports");
  CHECK(gltf.asset.version != NULL && strcmp(gltf.asset.version, "2.0") == 0, "asset version");
  CHECK(gltf.scene == 0 && gltf.sceneCount == 1, "default scene");
  CHECK(gltf.sceneCount == 1 && gltf.scenes[0].nodeCount == 2, "scene lists two nodes");
  REQUIRE(gltf.nodeCount == 2, "two nodes");
  CHECK(gltf.nodes[0].name != NULL && strcmp(gltf.nodes[0].name, "root") == 0, "node name");
  CHECK(gltf.nodes[0].translation.x == 1.5f && gltf.nodes[0].translation.z == 3.5f, "node translation");
  CHECK(gltf.nodes[0].scale.y == 2.0f, "node scale");
  CHECK(gltf.nodes[0].childCount == 1 && gltf.nodes[0].children[0] == 1, "node children");
  CHECK(fabs(gltf.nodes[1].rotation.z - 0.7071068) < 1e-6, "node rotation");
  CHECK(gltf.nodes[1].scale.x == 1.0f && gltf.nodes[1].translation.x == 0.0f, "default scale and translation");
  CHECK(gltf.nodes[1].mesh == U64_MAX, "no mesh is U64_MAX");
  REQUIRE(gltf.bufferCount == 1 && gltf.buffers[0].size == 24, "buffer size");
  CHECK(memcmp(gltf.buffers[0].buffer, k_positions, 24) == 0, "buffer contents loaded from the .bin");
  CHECK(gltf.bufferViewCount == 1 && gltf.bufferViews[0].type == GLTF_VERTEX_ARRAY, "buffer view target");
  REQUIRE(gltf.accessorCount == 1, "one accessor");
  CHECK(gltf.accessors[0].max_count == 3 && gltf.accessors[0].max[2].dFloat == 6.0, "accessor max");
  vec3 out[2] = {0};
  CHECK(gltf_accessor_get_vec3_array(&gltf, 0, out) && out[1].y == 5, "accessor data through the imported model");
  free_gltf(&gltf);
}

static void test_missing_gltf_file(void) {
  gltf_object gltf = {0};
  CHECK(import_gltf("vt_gltf_does_not_exist.gltf", &gltf) == FALSE, "missing file returns FALSE");
}

static void test_missing_asset_version_rejected(void) {
  REQUIRE(write_model("{\"asset\": {\"generator\": \"x\"}}", k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  CHECK(import_gltf(GLTF_PATH, &gltf) == FALSE, "asset without a version is rejected");
  remove_model();
}

static void test_required_extension_rejected(void) {
  REQUIRE(write_model("{\"asset\": {\"version\": \"2.0\"}, \"extensionsRequired\": [\"KHR_draco_mesh_compression\"]}", k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  CHECK(import_gltf(GLTF_PATH, &gltf) == FALSE, "unsupported required extension is rejected");
  remove_model();
}

// Tier 2 (via the JSON float loader): integer-valued arrays fall back to defaults.
static void test_integer_node_values(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"nodes\": [{\"translation\": [1, 2, 3], \"scale\": [2, 2, 2], \"rotation\": [0, 0, 0, 1]}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  REQUIRE(ok == TRUE && gltf.nodeCount == 1, "model imports");
  CHECK(gltf.nodes[0].translation.x == 1 && gltf.nodes[0].translation.y == 2 && gltf.nodes[0].translation.z == 3, "integer translation is read");
  CHECK(gltf.nodes[0].scale.x == 2, "integer scale is read");
  free_gltf(&gltf);
}

// Tier 2: glTF booleans are read as integers, so "normalized": true comes out FALSE.
static void test_accessor_normalized_flag(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"buffers\": [{\"uri\": \"" BIN_PATH "\", \"byteLength\": 24}],"
    " \"bufferViews\": [{\"buffer\": 0, \"byteLength\": 24}],"
    " \"accessors\": [{\"bufferView\": 0, \"componentType\": 5121, \"normalized\": true, \"count\": 4, \"type\": \"SCALAR\"}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  REQUIRE(ok == TRUE && gltf.accessorCount == 1, "model imports");
  CHECK(gltf.accessors[0].normalized == TRUE, "\"normalized\": true is read as TRUE");
  free_gltf(&gltf);
}

// Tier 3: import_gltf leaks uriPath, and the JSON it parses leaks its names and file buffer.
static void test_import_then_free_returns_all_memory(void) {
  REQUIRE(write_model(k_basic_gltf, k_positions, sizeof(k_positions)), "write model files");
  u64 before = get_memory_allocated();
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  REQUIRE(ok == TRUE, "model imports");
  free_gltf(&gltf);
  CHECK(get_memory_allocated() == before, "import_gltf + free_gltf frees everything");
}

/*
 * bug-report #18: the buffer copy trusts byteLength, so a .bin shorter than
 * it claims is read past its end. Import should fail instead.
 */
static void test_bin_shorter_than_byte_length(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"buffers\": [{\"uri\": \"" BIN_PATH "\", \"byteLength\": 4096}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == FALSE, "buffer whose byteLength exceeds the .bin size is rejected");
}

/*
 * bug-report #16: import_gltf starts one thread per object into a
 * thrd_t[512] on the stack with no bounds check.
 */
static void test_more_than_512_objects(void) {
  const u64 node_count = 600;
  u64 cap = 64 + node_count * 4;
  char* json = vallocate(cap, MEMORY_TAG_APPLICATION);
  u64 len = (u64)snprintf(json, cap, "{\"asset\": {\"version\": \"2.0\"}, \"nodes\": [");
  for(u64 i = 0; i < node_count; i++) {
    len += (u64)snprintf(json + len, cap - len, i == 0 ? "{}" : ",{}");
  }
  snprintf(json + len, cap - len, "]}");
  b8 written = write_model(json, k_positions, sizeof(k_positions));
  vfree(json, cap, MEMORY_TAG_APPLICATION);
  REQUIRE(written, "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == TRUE, "model with 600 nodes imports");
  CHECK(gltf.nodeCount == node_count, "all 600 nodes loaded");
  if(ok) {
    free_gltf(&gltf);
  }
}

void vgltf_test(void) {
  RUN_TEST(test_packed_vec3);
  RUN_TEST(test_buffer_view_offset);
  RUN_TEST(test_accessor_byte_offset);
  RUN_TEST(test_interleaved_attributes);
  RUN_TEST(test_accessor_past_view_rejected);
  RUN_TEST(test_accessor_past_buffer_rejected);
  RUN_TEST(test_bad_indices_rejected);
  RUN_TEST(test_wrong_type_rejected);
  RUN_TEST(test_index_arrays_widen_to_u32);
  RUN_TEST(test_non_float_vector_accessor);
  // The JSON parser under import_gltf leaks (Tier 3), checked separately below.
  RUN_TEST_IGNORE_LEAKS(test_import_basic_model);
  RUN_TEST_IGNORE_LEAKS(test_missing_gltf_file);
  RUN_TEST_IGNORE_LEAKS(test_missing_asset_version_rejected);
  RUN_TEST_IGNORE_LEAKS(test_required_extension_rejected);
  RUN_TEST_IGNORE_LEAKS(test_integer_node_values);
  RUN_TEST_IGNORE_LEAKS(test_accessor_normalized_flag);
  RUN_TEST(test_import_then_free_returns_all_memory);
  RUN_TEST_IGNORE_LEAKS(test_bin_shorter_than_byte_length);
  RUN_TEST_IGNORE_LEAKS(test_more_than_512_objects);
}
