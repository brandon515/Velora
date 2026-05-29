#pragma once

#include "core/utils/vgltf.h"
#include "defines.h"
#include "core/utils/vmath.h"
#include "ecs/vtransform.h"

typedef struct _vjoint{
  vtransform localTransform;
  mat4 inverseBindMatrix;
}vjoint;

typedef struct _vskin{
  u64 jointCount;
  vjoint* joints;
}vskin;

b8 vskin_from_gltf(gltf_object* gltf, u64 skinIndex, vskin* outSkin);

void free_vskin(vskin* skin);