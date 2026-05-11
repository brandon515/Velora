#include "vmesh.h"
#include "utils/vgltf.h"
#include "core/vmemory.h"
#include "core/logger.h"

b8 vmesh_from_gltf(gltf_object *obj, u64 meshIndex, vmesh *outMesh){
  if(meshIndex >= obj->meshCount){
    return FALSE;
  }
  vmesh scratchMesh = {0};
  gltf_mesh mesh = obj->meshes[meshIndex];
  for(int i = 0; i < mesh.primitiveCount; i++){
    gltf_mesh_primitive prim = mesh.primitives[i];
    if(prim.mode != GLTF_MODE_TRIANGLES){
      VERROR("Primitive %d in Mesh %d is not a triangle mesh mode, Velora currently only supports triangles", i, meshIndex);
      return FALSE;
    }
    if(prim.attributes.position == U64_MAX){
      VERROR("Primitive %d in Mesh %d does not contain a position for the vertices. Velora currently does not support positionless primitives", i, meshIndex);
      return FALSE;
    }
    VEL_CHECK(gltf_accessor_get_element_count(obj, prim.attributes.position, &scratchMesh.vertexCount));
    scratchMesh.vertices = vallocate(scratchMesh.vertexCount*sizeof(vertex), MEMORY_TAG_RENDERER);
    vec3 positions[scratchMesh.vertexCount];
    VEL_CHECK(gltf_accessor_get_vec3_array(obj, prim.attributes.position, positions));
    vec3 normals[scratchMesh.vertexCount];
    vset_memory(normals, 0, sizeof(vec3)*scratchMesh.vertexCount);
    gltf_accessor_get_vec3_array(obj, prim.attributes.normal, normals);
    vec2 texCoords[scratchMesh.vertexCount];
    vset_memory(texCoords, 0, sizeof(vec2)*scratchMesh.vertexCount);
    if(prim.attributes.texCount >= 1){
      gltf_accessor_get_vec2_array(obj, prim.attributes.texCoords[0], texCoords);
    }
    for(int i = 0; i < scratchMesh.vertexCount; i++){
      scratchMesh.vertices[i].pos = positions[i];
      scratchMesh.vertices[i].normal = normals[i];
      scratchMesh.vertices[i].texCoord = texCoords[i];
    }
    VEL_CHECK(gltf_accessor_get_element_count(obj, prim.indicies, &scratchMesh.indexCount));
    scratchMesh.indicies = vallocate(scratchMesh.indexCount*sizeof(u32), MEMORY_TAG_RENDERER);
    if(gltf_accessor_get_u32_array(obj, prim.indicies, scratchMesh.indicies) == FALSE){
      vfree(scratchMesh.indicies, scratchMesh.indexCount*sizeof(u32), MEMORY_TAG_RENDERER);
      return FALSE;
    }
    vmaterial_from_gltf(obj, prim.material, &scratchMesh.material);
    (*outMesh) = scratchMesh;
  }
  return TRUE;
}

void free_vmesh(vmesh *mesh){
  vfree(mesh->vertices, mesh->vertexCount*sizeof(vertex), MEMORY_TAG_RENDERER);
  vfree(mesh->indicies, mesh->indexCount*sizeof(u32), MEMORY_TAG_RENDERER);
  free_vmaterial(&mesh->material);
}