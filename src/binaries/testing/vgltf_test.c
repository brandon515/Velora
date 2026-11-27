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

// #40: vector accessors that aren't floats (e.g. JOINTS_0 as unsigned byte VEC4) can be read.
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

// #40: JOINTS_0 may also be unsigned short VEC4. Read as vec4 the indices are widened to float.
static void test_u16_vec4_joints(void) {
  u16 joints[8] = {0, 1, 2, 300, 4, 5, 6, 65535};
  fake_gltf f;
  fake_init(&f, joints, sizeof(joints), GLTF_U16, "VEC4", 2);
  gltf_data_stream stream = {0};
  REQUIRE(gltf_get_stream(&f.gltf, 0, &stream) == TRUE, "unsigned short VEC4 accessor can be streamed");
  CHECK(stream.dataSize == 4 * sizeof(u16) && stream.count == 2, "8 bytes per element");
  gltf_free_stream(stream);
  vec4 out[2] = {0};
  REQUIRE(gltf_accessor_get_vec4_array(&f.gltf, 0, out) == TRUE, "unsigned short VEC4 reads as vec4");
  CHECK(out[0].x == 0 && out[0].w == 300, "first element widened to float");
  CHECK(out[1].x == 4 && out[1].w == 65535, "second element widened to float");
}

// #40: WEIGHTS_0 may be normalized unsigned byte VEC4, so c / 255 becomes the float value.
static void test_normalized_u8_vec4_weights(void) {
  u8 weights[8] = {255, 0, 0, 0, 128, 127, 0, 0};
  fake_gltf f;
  fake_init(&f, weights, sizeof(weights), GLTF_U8, "VEC4", 2);
  f.accessors[0].normalized = TRUE;
  vec4 out[2] = {0};
  REQUIRE(gltf_accessor_get_vec4_array(&f.gltf, 0, out) == TRUE, "normalized unsigned byte VEC4 reads as vec4");
  CHECK_NEAR(out[0].x, 1.0, 1e-6, "255 maps to 1.0");
  CHECK_NEAR(out[0].y, 0.0, 1e-6, "0 maps to 0.0");
  CHECK_NEAR(out[1].x, 128.0 / 255.0, 1e-6, "128 maps to 128/255");
  CHECK_NEAR(out[1].y, 127.0 / 255.0, 1e-6, "127 maps to 127/255");
}

// #40: TEXCOORD_0 may be normalized unsigned short VEC2, so c / 65535 becomes the float value.
static void test_normalized_u16_vec2_texcoords(void) {
  u16 uvs[4] = {0, 65535, 32768, 16384};
  fake_gltf f;
  fake_init(&f, uvs, sizeof(uvs), GLTF_U16, "VEC2", 2);
  f.accessors[0].normalized = TRUE;
  vec2 out[2] = {0};
  REQUIRE(gltf_accessor_get_vec2_array(&f.gltf, 0, out) == TRUE, "normalized unsigned short VEC2 reads as vec2");
  CHECK_NEAR(out[0].x, 0.0, 1e-6, "0 maps to 0.0");
  CHECK_NEAR(out[0].y, 1.0, 1e-6, "65535 maps to 1.0");
  CHECK_NEAR(out[1].x, 32768.0 / 65535.0, 1e-6, "32768 maps to 32768/65535");
  CHECK_NEAR(out[1].y, 16384.0 / 65535.0, 1e-6, "16384 maps to 16384/65535");
}

// #40: signed normalized values map to max(c / 127, -1), so both -128 and -127 become -1.0.
static void test_normalized_i8_vec2(void) {
  i8 data[4] = {-128, 127, -127, 0};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_I8, "VEC2", 2);
  f.accessors[0].normalized = TRUE;
  vec2 out[2] = {0};
  REQUIRE(gltf_accessor_get_vec2_array(&f.gltf, 0, out) == TRUE, "normalized signed byte VEC2 reads as vec2");
  CHECK_NEAR(out[0].x, -1.0, 1e-6, "-128 clamps to -1.0");
  CHECK_NEAR(out[0].y, 1.0, 1e-6, "127 maps to 1.0");
  CHECK_NEAR(out[1].x, -1.0, 1e-6, "-127 maps to -1.0");
  CHECK_NEAR(out[1].y, 0.0, 1e-6, "0 maps to 0.0");
}

// #40: an interleaved u8 VEC4 with byteStride 8 takes 4 bytes per element and skips the padding.
static void test_strided_u8_vec4(void) {
  u8 data[16] = {
    1, 2, 3, 4,     99, 99, 99, 99,  // element 0, then padding
    5, 6, 7, 8,     99, 99, 99, 99,  // element 1, then padding
  };
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_U8, "VEC4", 2);
  f.views[0].stride = 8;
  gltf_data_stream stream = {0};
  REQUIRE(gltf_get_stream(&f.gltf, 0, &stream) == TRUE, "strided unsigned byte VEC4 can be streamed");
  CHECK(stream.dataSize == 4 && stream.count == 2, "4 bytes per element despite stride 8");
  CHECK(stream.data[0] == 1 && stream.data[3] == 4, "first element read");
  CHECK(stream.data[4] == 5 && stream.data[7] == 8, "second element read from the stride, not the padding");
  gltf_free_stream(stream);
}

// #40: matrices stay float-only. Non-float matrices need column padding (spec 3.6.2.4), which isn't supported.
static void test_non_float_matrix_rejected(void) {
  u8 data[64] = {0};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_U8, "MAT4", 1);
  u64 size = 0;
  CHECK(gltf_accessor_get_element_size(&f.gltf, 0, &size, NULL) == FALSE, "unsigned byte MAT4 is rejected");
  mat4 out = {0};
  CHECK(gltf_accessor_get_mat4_array(&f.gltf, 0, &out) == FALSE, "unsigned byte MAT4 is not read as mat4");
}

// Signed bytes keep their sign when widened to i16 or i32.
static void test_i8_widens_with_sign(void) {
  i8 data[3] = {-1, -128, 127};
  fake_gltf f;
  fake_init(&f, data, sizeof(data), GLTF_I8, "SCALAR", 3);
  i16 out16[3] = {0};
  REQUIRE(gltf_accessor_get_i16_array(&f.gltf, 0, out16) == TRUE, "i8 accessor reads as i16");
  CHECK(out16[0] == -1 && out16[1] == -128 && out16[2] == 127, "i8 values keep their sign as i16");
  i32 out32[3] = {0};
  REQUIRE(gltf_accessor_get_i32_array(&f.gltf, 0, out32) == TRUE, "i8 accessor reads as i32");
  CHECK(out32[0] == -1 && out32[1] == -128 && out32[2] == 127, "i8 values keep their sign as i32");
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

// import_gltf followed by free_gltf returns every byte. RUN_TEST also checks each memory tag.
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
  CHECK(gltf.buffers == NULL && gltf.bufferCount == 0 && gltf.asset.version == NULL, "a failed import leaves the object empty");
}

/*
 * Touches every object type the importer allocates for (names, defaults,
 * morph targets, skins, animations, cameras...). RUN_TEST checks the total
 * and each memory tag, so a block freed under the wrong tag or size fails.
 * The skinning accessors also cover #40 through a real import.
 */
static const char* k_full_gltf =
  "{\"asset\": {\"version\": \"2.0\", \"generator\": \"velora tests\", \"copyright\": \"none\", \"minVersion\": \"2.0\"},"
  " \"extensionsUsed\": [\"VT_unknown_extension\"],"
  " \"scene\": 0,"
  " \"scenes\": [{\"name\": \"main\", \"nodes\": [0]}],"
  " \"nodes\": ["
  "   {\"name\": \"root\", \"mesh\": 0, \"skin\": 0, \"children\": [1], \"weights\": [0.5]},"
  "   {\"name\": \"joint\", \"camera\": 0, \"matrix\": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]}"
  " ],"
  " \"meshes\": [{\"name\": \"mesh\", \"weights\": [0.5], \"primitives\": [{"
  "   \"attributes\": {\"POSITION\": 0, \"JOINTS_0\": 1, \"WEIGHTS_0\": 2, \"TEXCOORD_0\": 0, \"COLOR_0\": 0},"
  "   \"material\": 0, \"targets\": [{\"POSITION\": 0}]}]}],"
  " \"skins\": [{\"name\": \"skin\", \"joints\": [0, 1]}],"
  " \"animations\": [{\"name\": \"anim\","
  "   \"channels\": [{\"sampler\": 0, \"target\": {\"node\": 1, \"path\": \"translation\"}}],"
  "   \"samplers\": [{\"input\": 3, \"output\": 0}]}],"
  " \"cameras\": [{\"name\": \"cam\", \"type\": \"perspective\", \"perspective\": {\"yfov\": 1.0, \"znear\": 0.1}}],"
  " \"materials\": [{\"name\": \"mat\", \"alphaMode\": \"BLEND\", \"emissiveFactor\": [1, 0, 0],"
  "   \"pbrMetallicRoughness\": {\"baseColorFactor\": [1, 1, 1, 1]}}, {}],"
  " \"samplers\": [{\"name\": \"smp\"}],"
  " \"textures\": [{\"name\": \"tex\", \"source\": 0, \"sampler\": 0}],"
  " \"buffers\": [{\"name\": \"buf\", \"uri\": \"" BIN_PATH "\", \"byteLength\": 48}],"
  " \"bufferViews\": [{\"name\": \"view\", \"buffer\": 0, \"byteLength\": 48}],"
  " \"accessors\": ["
  "   {\"name\": \"positions\", \"bufferView\": 0, \"componentType\": 5126, \"count\": 2, \"type\": \"VEC3\"},"
  "   {\"name\": \"joints\", \"bufferView\": 0, \"byteOffset\": 24, \"componentType\": 5121, \"count\": 2, \"type\": \"VEC4\"},"
  "   {\"name\": \"weights\", \"bufferView\": 0, \"byteOffset\": 32, \"componentType\": 5121, \"normalized\": true, \"count\": 2, \"type\": \"VEC4\"},"
  "   {\"name\": \"times\", \"bufferView\": 0, \"byteOffset\": 40, \"componentType\": 5126, \"count\": 2, \"type\": \"SCALAR\", \"max\": [1.0], \"min\": [0.0]}"
  " ]}";

static void write_full_bin(u8 bin[48]) {
  const f32 positions[6] = {1, 2, 3, 4, 5, 6};
  const u8 joints[8] = {0, 1, 0, 0, 1, 0, 0, 0};
  const u8 weights[8] = {255, 0, 0, 0, 128, 127, 0, 0};
  const f32 times[2] = {0, 1};
  memcpy(bin, positions, sizeof(positions));
  memcpy(bin + 24, joints, sizeof(joints));
  memcpy(bin + 32, weights, sizeof(weights));
  memcpy(bin + 40, times, sizeof(times));
}

static void test_import_full_model(void) {
  u8 bin[48];
  write_full_bin(bin);
  REQUIRE(write_model(k_full_gltf, bin, sizeof(bin)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  REQUIRE(ok == TRUE, "full model imports");
  CHECK(gltf.meshCount == 1 && gltf.meshes[0].primitiveCount == 1 && gltf.meshes[0].primitives[0].morphCount == 1, "mesh with a morph target");
  CHECK(gltf.skinCount == 1 && gltf.skins[0].jointCount == 2, "skin joints");
  CHECK(gltf.animationCount == 1 && gltf.animations[0].channelCount == 1 && gltf.animations[0].samplerCount == 1, "animation channel and sampler");
  CHECK(gltf.cameraCount == 1 && gltf.materialCount == 2 && gltf.textureCount == 1 && gltf.samplerCount == 1, "camera, materials, texture, sampler");

  gltf_mesh_primitive prim = gltf.meshes[0].primitives[0];
  REQUIRE(prim.attributes.jointCount == 1 && prim.attributes.weightCount == 1, "primitive has JOINTS_0 and WEIGHTS_0");
  vec4 joints[2] = {0};
  REQUIRE(gltf_accessor_get_vec4_array(&gltf, prim.attributes.joints[0], joints) == TRUE, "imported JOINTS_0 reads as vec4");
  CHECK(joints[0].x == 0 && joints[0].y == 1 && joints[1].x == 1, "joint indices");
  vec4 weights[2] = {0};
  REQUIRE(gltf_accessor_get_vec4_array(&gltf, prim.attributes.weights[0], weights) == TRUE, "imported WEIGHTS_0 reads as vec4");
  CHECK_NEAR(weights[0].x, 1.0, 1e-6, "weight 255 maps to 1.0");
  CHECK_NEAR(weights[1].x + weights[1].y, 1.0, 1e-6, "weights 128 and 127 sum to 1.0");
  free_gltf(&gltf);
}

// Fails after buffers, views and accessors are already loaded, so all of them must be cleaned up.
static void test_bad_node_matrix_rejected(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\", \"generator\": \"velora tests\"},"
    " \"buffers\": [{\"name\": \"buf\", \"uri\": \"" BIN_PATH "\", \"byteLength\": 24}],"
    " \"bufferViews\": [{\"buffer\": 0, \"byteLength\": 24}],"
    " \"accessors\": [{\"bufferView\": 0, \"componentType\": 5126, \"count\": 2, \"type\": \"VEC3\", \"max\": [4, 5, 6]}],"
    " \"nodes\": [{\"name\": \"ok\", \"children\": [1]}, {\"name\": \"bad\", \"matrix\": [1, 0, 0]}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == FALSE, "node matrix without 16 floats is rejected");
  CHECK(gltf.nodes == NULL && gltf.accessors == NULL, "a failed import leaves the object empty");
}

static void test_illegal_camera_type_rejected(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"cameras\": [{\"name\": \"cam\", \"type\": \"fisheye\"}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == FALSE, "camera with an unknown type is rejected");
}

static void test_missing_bin_file_rejected(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"buffers\": [{\"name\": \"buf\", \"uri\": \"vt_gltf_missing.bin\", \"byteLength\": 24}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == FALSE, "buffer whose .bin doesn't exist is rejected");
}

static void test_data_uri_buffer_rejected(void) {
  const char* json =
    "{\"asset\": {\"version\": \"2.0\"},"
    " \"buffers\": [{\"uri\": \"data:application/octet-stream;base64,AAAA\", \"byteLength\": 3}]}";
  REQUIRE(write_model(json, k_positions, sizeof(k_positions)), "write model files");
  gltf_object gltf = {0};
  b8 ok = import_gltf(GLTF_PATH, &gltf);
  remove_model();
  CHECK(ok == FALSE, "data URI buffer is rejected (not supported yet)");
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
  RUN_TEST(test_u16_vec4_joints);
  RUN_TEST(test_normalized_u8_vec4_weights);
  RUN_TEST(test_normalized_u16_vec2_texcoords);
  RUN_TEST(test_normalized_i8_vec2);
  RUN_TEST(test_strided_u8_vec4);
  RUN_TEST(test_non_float_matrix_rejected);
  RUN_TEST(test_i8_widens_with_sign);
  RUN_TEST(test_import_basic_model);
  RUN_TEST(test_missing_gltf_file);
  RUN_TEST(test_missing_asset_version_rejected);
  RUN_TEST(test_required_extension_rejected);
  RUN_TEST(test_integer_node_values);
  RUN_TEST(test_accessor_normalized_flag);
  RUN_TEST(test_import_then_free_returns_all_memory);
  RUN_TEST(test_bin_shorter_than_byte_length);
  RUN_TEST(test_import_full_model);
  RUN_TEST(test_bad_node_matrix_rejected);
  RUN_TEST(test_illegal_camera_type_rejected);
  RUN_TEST(test_missing_bin_file_rejected);
  RUN_TEST(test_data_uri_buffer_rejected);
  RUN_TEST(test_more_than_512_objects);
}
