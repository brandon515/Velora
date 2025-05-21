#pragma once
#include "defines.h"
#include "utils/vmath.h"
#include "utils/vimport.h"
#include <threads.h>

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
  u64 bufferIndex; // The buffer being referenced
  u64 offset; // The offset in bytes
  u64 stride; // The stride by which the data should be traversed, if this is 0 then use the size of the accessor type
  u64 type; // Either a GLTF_VERTEX_ARRAY or GLTF_INDEX_ARRAY
  char *name;
}gltf_buffer_view;

typedef union _gltf_value{
  u64 unsignedInteger;
  i64 signedInteger;
  f64 dFloat;
}gltf_value;

typedef struct _gltf_sparse_accessor_details{
  u64 bufferViewIndex;
  u64 byteOffset;
  u64 cType;
}gltf_sparse_accessor_details;

typedef struct _gltf_sparse_accessor{
  u64 count;
  gltf_sparse_accessor_details indices;
  gltf_sparse_accessor_details values;
}gltf_sparse_accessor;

typedef struct _gltf_accessor{
  u64 bufferViewIndex; // The attached data, if this is U64_MAX than it's just a count full of zeros. Sparse may exist to override any values
  u64 offset; // The offset in the buffer view itself
  u64 componentType; // Whether the SCALAR or VEC3 is made up of floats or integers
  b8 normalized;// Whether the data is already normalized, (0,1) for unsigned and (-1,1) for signed
  u64 count; // How many of the variable there is, data can be interlaced to be sure to increment by stride in the bufferview
  char *type; // Type of accessor, possible types are MAT2, MAT3, MAT4, VEC2, VEC3, VEC4, or SCALAR
  gltf_value *max; // Optional value, NULL if it doesn't exist. Maximum value for each section
  u64 max_count; // This is 0 if there are no max values
  gltf_value *min; // Optional value, NULL if it doesn't exist. Minimum value for each section
  u64 min_count;// This is 0 if there are no min values
  gltf_sparse_accessor sparse;// This is dumb and I hate it but it contains data to be over written if it exists
  char *name;// The name of the accessor, not mandatory
}gltf_accessor;

typedef struct _gltf_texture{
  u64 sampler;
  u64 source;
  char *name;
}gltf_texture;

typedef struct _gltf_texture_info{
  u64 textureIndex;
  u64 texCoordIndex;
  f64 scaleStrength;
}gltf_texture_info;

typedef struct _gltf_metal_roughness{
  vec4 baseColor;
  f64 metallicFactor;
  f64 roughnessFactor;
  gltf_texture_info baseColorTexture;
  gltf_texture_info metallicRoughnessTexture;
}gltf_metal_roughness;

typedef struct _gltf_material{
  char *name;
  vec3 emissiveFactor;
  char *alphaMode;
  f64 alphaCutoff;
  b8 doubleSided;
  gltf_metal_roughness pbrMetallicRoughness;
  gltf_texture_info normalTexture;
  gltf_texture_info occlusionTexture;
  gltf_texture_info emissiveTexture;
}gltf_material;

typedef struct _gltf_image{
  velora_pixels uriData;
  char *mimeType;
  u64 bufferViewIndex;
  char *name;
}gltf_image;

typedef struct _gltf_object{
  gltf_buffer *buffers;
  u64 bufferCount;
  gltf_buffer_view *bufferViews;
  u64 bufferViewCount;
  gltf_accessor *accessors;
  u64 accessorCount;
  gltf_image *images;
  u64 imageCount;
  gltf_material *materials;
  u64 materialCount;
  gltf_texture *textures;
  u64 textureCount;
}gltf_object;

typedef struct _import_thread_tracking{
  mtx_t *mutexes;
  b8 *results;
  u64 threadCount;
}import_thread_tracking;

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