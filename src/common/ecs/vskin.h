#pragma once

#include "core/utils/vgltf.h"
#include "defines.h"
#include "ecs/ecs_api.h"
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

VECS_API b8 vskin_from_gltf(gltf_object* gltf, u64 skinIndex, vskin* outSkin);

VECS_API void free_vskin(vskin* skin);