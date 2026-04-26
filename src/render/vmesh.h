#pragma once

#include "defines.h"
#include "utils/vmath.h"
#include "utils/vgltf.h"

typedef struct _vertex {
  vec3 pos;
  vec3 normal;
  vec2 texCoord;
} vertex;

typedef struct _vmesh {
  vertex *vertices;
  u64 vertexCount;
  u32 *indicies;
  u64 indexCount;
} vmesh;

b8 vmesh_from_gltf(gltf_object *obj, u64 meshIndex, vmesh *outMesh);