#include "vmesh.h"
#include "utils/vgltf.h"

b8 vmesh_from_gltf(gltf_object *obj, u64 meshIndex, vmesh *outMesh){
  if(meshIndex >= obj->meshCount){
    return FALSE;
  }
  /*vmesh scratchMesh = {0};
  gltf_mesh mesh = obj->meshes[meshIndex];
  for(int i = 0; i < mesh.primitiveCount; i++){
    gltf_mesh_primitive prim = mesh.primitives[i];
    if(prim.indicies >= obj->accessorCount){
      return FALSE;
    }
    gltf_accessor indexAcc = obj->accessors[prim.indicies];
    if(indexAcc.bufferViewIndex >= obj->bufferViewCount){
      return FALSE;
    }
    indexAcc.bufferViewIndex;
  }*/
  return TRUE;
}