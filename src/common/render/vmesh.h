#pragma once

#include "defines.h"
#include "core/utils/vmath.h"
#include "core/utils/vgltf.h"
#include "render/vmaterial.h"

typedef struct _vertex {
  vec3 pos;
  vec3 normal;
  vec2 texCoord;
  vec4 weights;
  vec4 joints;
} vertex;

typedef struct _vmesh {
  vertex *vertices;
  u64 vertexCount;
  u32 *indicies;
  u64 indexCount;
  vmaterial material;
} vmesh;

b8 vmesh_from_gltf(gltf_object *obj, u64 meshIndex, vmesh *outMesh);

void free_vmesh(vmesh *mesh);