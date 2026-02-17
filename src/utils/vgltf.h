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

#define GLTF_MODE_POINTS 0
#define GLTF_MODE_LINES 1
#define GLTF_MODE_LINE_LOOP 2
#define GLTF_MODE_LINE_STRIP 3
#define GLTF_MODE_TRIANGLES 4
#define GLTF_MODE_TRIANGLES_STRIP 5
#define GLTF_MODE_TRIANGLES_FAN 6

#define GLTF_SAMPLER_FILTER_NEAREST 9728
#define GLTF_SAMPLER_FILTER_LINEAR 9729
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST 9784
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST 9785
#define GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR 9786
#define GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR 9787

#define GLTF_SAMPLER_WRAP_CLAMP_TO_EDGE 33071
#define GLTF_SAMPLER_WRAP_MIRRORED_REPEAT 33648
#define GLTF_SAMPLER_WRAP_REPEAT 10497

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

typedef struct _gltf_mesh_primitive_attribute{
  u64 position;
  u64 normal;
  u64 tangent;
  u64 *texCoords;
  u64 texCount;
  u64 *colors;
  u64 colorCount;
  u64 *joints;
  u64 jointCount;
  u64 *weights;
  u64 weightCount;
}gltf_mesh_primitive_attribute;

typedef struct _gltf_mesh_primitive{
  gltf_mesh_primitive_attribute attributes;
  u64 indicies;
  u64 material;
  gltf_mesh_primitive_attribute *morphTargets;
  u64 morphCount;
  u64 mode;
}gltf_mesh_primitive;

typedef struct _gltf_mesh{
  gltf_mesh_primitive *primitives;
  u64 primitiveCount;
  f64 *weights;
  u64 weightCount;
  char *name;
}gltf_mesh;

typedef struct _gltf_animation_sampler{
  u64 input;
  char *interpolation;
  u64 output;
}gltf_animation_sampler;

typedef struct _gltf_animation_channel_target{
  u64 node;
  char *path;
}gltf_animation_channel_target;

typedef struct _gltf_animation_channel{
  u64 sampler;
  gltf_animation_channel_target target;
}gltf_animation_channel;

typedef struct _gtlf_animation{
  gltf_animation_channel *channels;
  u64 channelCount;
  gltf_animation_sampler *samplers;
  u64 samplerCount;
  char *name;
}gltf_animation;

typedef struct _gltf_camera_perspective{
  f64 aspectRatio;
  f64 yfov;
  f64 zfar;
  f64 znear;
}gltf_camera_perspective;

typedef struct _gltf_camera_orthographic{
  f64 xmag;
  f64 ymag;
  f64 zfar;
  f64 znear;
}gltf_camera_orthographic;

typedef struct _gltf_camera{
  gltf_camera_perspective perspective;
  gltf_camera_orthographic orthographic;
  char *type;
  char *name;
}gltf_camera;

typedef struct _gltf_skin{
  u64 inverseBindMatrices;
  u64 skeleton;
  u64 *joints;
  u64 jointCount;
  char *name;
}gltf_skin;

typedef struct _gltf_asset{
  char *copyright;
  char *generator;
  char *version;
  char *minVersion;
}gltf_asset;

typedef struct _gltf_sampler{
  u64 magFilter;
  u64 minFilter;
  u64 wrapS;
  u64 wrapT;
  char *name;
}gltf_sampler;

typedef struct _gltf_node{
  u64 camera;
  u64 *children;
  u64 childCount;
  u64 skin;
  mat4 matrix;
  u64 mesh;
  quat rotation;
  vec3 scale;
  vec3 translation;
  f64 *weights;
  u64 weightCount;
  char *name;
}gltf_node;

typedef struct _gltf_scene{
  u64 *nodes;
  u64 nodeCount;
  char *name;
}gltf_scene;

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
  gltf_mesh *meshes;
  u64 meshCount;
  gltf_animation *animations;
  u64 animationCount;
  gltf_camera *cameras;
  u64 cameraCount;
  gltf_skin *skins;
  u64 skinCount;
  gltf_sampler *samplers;
  u64 samplerCount;
  gltf_node *nodes;
  u64 nodeCount;
  gltf_scene *scenes;
  u64 sceneCount;
  gltf_asset asset;
  u64 scene;
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
 b8 import_gltf(const char *uri, gltf_object *out_gltf);

/**
 * @brief Frees the memory that the gltf_object uses but not the pointer itself
 * @param out_gltf The gltf_object that's been filled with import_gltf
 */
 void free_gltf(gltf_object* out_gltf);