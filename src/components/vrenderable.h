#pragma once

#include "defines.h"
#include "utils/vmath.h"

typedef struct _vertex {
  vec3 pos;
  vec3 color;
  vec2 texCoord;
} vertex;

typedef struct _vmaterial {
  char *textureUri;
  void *renderMaterial;
} vmaterial;

typedef struct _vmesh {
  vertex *vertices;
  u64 vertexCount;
  u32 *indicies;
  u64 indexCount;
  void *renderMesh;
} vmesh;