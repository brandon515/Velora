#include "render/vmaterial.h"
#include "core/utils/vgltf.h"

b8 vmaterial_from_image(const char* baseColorURI, vmaterial *outMaterial){
  VEL_CHECK(import_pixels(baseColorURI, &outMaterial->baseColor));
  return TRUE;
}

b8 vmaterial_from_gltf(gltf_object *gltf, u64 matIndex, vmaterial *outMaterial){
  if(matIndex >= gltf->materialCount){
    return FALSE;
  }
  u64 textureIndex = gltf->materials[matIndex].pbrMetallicRoughness.baseColorTexture.textureIndex;
  outMaterial->baseColor = gltf->images[gltf->textures[textureIndex].source].uriData;
  return TRUE;
}

void free_vmaterial(vmaterial* material){
  free_pixels(&material->baseColor);
}