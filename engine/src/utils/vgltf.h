#pragma once
#include "defines.h"
#include "utils/vmath.h"
#include "utils/vimport.h"

#define GLTF_VERTEX_ARRAY 34962
#define GLTF_INDEX_ARRAY 34963

#define GLTF_I8 5120
#define GLTF_U8 5121
#define GLTF_I16 5122
#define GLTF_U16 5123
#define GLTF_U32 5125
#define GLTF_FLOAT 5126

typedef struct _gltf_buffer{
  u64 size; // The size of the buffer in bytes
  u8* buffer; // The stream of bytes, the gltf_buffer serves as the owner of the data stream
  char *name;
}gltf_buffer;

typedef struct _gltf_buffer_view{
  u64 size; // Complete size of this part of the buffer
  u8* buffer; // The buffer being referenced
  u64 stride; // The stride by which the data should be traversed, if this is 0 then use the size of the accessor type
  u64 type; // Either a GLTF_VERTEX_ARRAY or GLTF_INDEX_ARRAY
}gltf_buffer_view;

typedef union _gltf_value{
  u64 unsignedInteger;
  i64 signedInteger;
  f64 dFloat;
}gltf_value;

typedef struct _gltf_accessor{
  gltf_buffer_view *bufferView; // The attached data
  u64 offset; // The offset in the buffer view itself
  u64 componentType; // Whether the SCALAR or VEC3 is made up of floats or integers
  u64 count; // How many of the variable there is, data can be interlaced to be sure to increment by stride in the bufferview
  char *type; // Type of accessor, possible types are MAT2, MAT3, MAT4, VEC2, VEC3, VEC4, or SCALAR
  gltf_value *max; // Optional value, NULL if it doesn't exist. Maximum value for each section
  u64 max_count; // This is 0 if there are no max values
  gltf_value *min; // Optional value, NULL if it doesn't exist. Minimum value for each section
  u64 min_count;// This is 0 if there are no min values
  b8 normalized;
}gltf_accessor;

typedef struct _gltf_texture{
  i64 sampler;
  i64 source;
  char *name;
}gltf_texture;

typedef struct _gltf_material_texture{
  i64 textureIndex;
  u64 texCoordIndex;
  f64 scale;
}gltf_material_texture;

typedef struct _gltf_metal_roughness{
  vec4 baseColor;
  f32 metallicFactor;
  f32 roughnessFactor;
  gltf_material_texture baseColorTexture;
  gltf_material_texture metallicRoughnessTexture;
}gltf_metal_roughness;

typedef struct _gltf_material{
  char *name;
  vec3 emissiveFactor;
  char *alphaMode;
  f32 alphaCutoff;
  b8 doubleSided;
  gltf_metal_roughness pbrMetallicRoughness;
  gltf_material_texture normalTexture;
  gltf_material_texture occlusionTexture;
  gltf_material_texture emissiveTexture;
}gltf_material;

typedef struct _gltf_object{
  gltf_buffer *buffers;
  u64 bufferCount;
  gltf_buffer_view *bufferViews;
  u64 bufferViewCount;
  gltf_accessor *accessors;
  u64 accessorCount;
  velora_pixels *images;
  u64 imageCount;
  gltf_material *materials;
  u64 materialCount;
  gltf_texture *textures;
  u64 textureCount;
}gltf_object;

/**
 * @brief imports the gltf file into a usable file
 * @param uri The filename of the gltf file
 * @param out_gltf A gltf_object variable to be filled with the data pointed at by the URI
 * @return FALSE if the file doesn't exist or the file isn't valid JSON, TRUE otherwise
 */
VAPI b8 import_gltf(const char *uri, gltf_object *out_gltf);

/**
 * @brief Frees the memory that the gltf_object uses but not the pointer itself
 * @param out_gltf The gltf_object that's been filled with import_gltf
 */
VAPI void free_gltf(gltf_object* out_gltf);