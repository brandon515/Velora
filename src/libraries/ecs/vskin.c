#include "ecs/vskin.h"
#include "core/utils/vgltf.h"
#include "core/utils/vmath.h"
#include "core/vmemory.h"
#include "core/logger.h"

b8 vskin_from_gltf(gltf_object* gltf, u64 skinIndex, vskin* outSkin){
  if(skinIndex >= gltf->skinCount){
    VERROR("Skin index %d is out of bounds, max index is %d", skinIndex, gltf->skinCount-1);
    return FALSE;
  }
  vskin scratchSkin = {0};
  gltf_skin skin = gltf->skins[skinIndex];
  scratchSkin.jointCount = skin.jointCount;
  scratchSkin.joints = vallocate(scratchSkin.jointCount*sizeof(vjoint), MEMORY_TAG_RENDERER);

  // Maps a gltf node index back to its position in the skin's joint array, U64_MAX if the node is not a joint
  u64 nodeToJoint[gltf->nodeCount];
  for(int i = 0; i < gltf->nodeCount; i++){
    nodeToJoint[i] = U64_MAX;
  }

  mat4 inverseBindMatrices[scratchSkin.jointCount];
  for(int i = 0; i < scratchSkin.jointCount; i++){
    inverseBindMatrices[i] = indentity_mat4();
  }
  if(skin.inverseBindMatrices != U64_MAX){
    u64 matrixCount = 0;
    VEL_CHECK(gltf_accessor_get_element_count(gltf, skin.inverseBindMatrices, &matrixCount));
    if(matrixCount != scratchSkin.jointCount){
      VERROR("Skin has %d joints but %d inverse bind matrices, the counts must match", scratchSkin.jointCount, matrixCount);
      vfree(scratchSkin.joints, scratchSkin.jointCount*sizeof(vjoint), MEMORY_TAG_RENDERER);
      return FALSE;
    }
    if(gltf_accessor_get_mat4_array(gltf, skin.inverseBindMatrices, inverseBindMatrices) == FALSE){
      vfree(scratchSkin.joints, scratchSkin.jointCount*sizeof(vjoint), MEMORY_TAG_RENDERER);
      return FALSE;
    }
  }

  for(int i = 0; i < scratchSkin.jointCount; i++){
    u64 nodeIndex = skin.joints[i];
    if(nodeIndex >= gltf->nodeCount){
      VERROR("Joint %d in the skin references node %d which is out of bounds, max index is %d", i, nodeIndex, gltf->nodeCount-1);
      vfree(scratchSkin.joints, scratchSkin.jointCount*sizeof(vjoint), MEMORY_TAG_RENDERER);
      return FALSE;
    }
    gltf_node node = gltf->nodes[nodeIndex];
    vec3 euler = quat_to_euler(node.rotation);
    scratchSkin.joints[i].localTransform = create_transform(node.translation.x, node.translation.y, node.translation.z,
                                                            euler.x, euler.y, euler.z,
                                                            node.scale.x, node.scale.y, node.scale.z,
                                                            NULL);
    scratchSkin.joints[i].inverseBindMatrix = inverseBindMatrices[i];
    nodeToJoint[nodeIndex] = i;
  }

  // glTF stores the hierarchy through each node's children, so a joint's local transform is relative to its
  // parent joint. Walk each joint's children and point the child joint transforms back at their parent so the
  // global joint transform can be accumulated up the chain.
  for(int i = 0; i < scratchSkin.jointCount; i++){
    gltf_node node = gltf->nodes[skin.joints[i]];
    for(int c = 0; c < node.childCount; c++){
      if(node.children[c] >= gltf->nodeCount){
        continue;
      }
      u64 childJoint = nodeToJoint[node.children[c]];
      if(childJoint != U64_MAX){
        scratchSkin.joints[childJoint].localTransform.parent = &scratchSkin.joints[i].localTransform;
      }
    }
  }

  vcopy_memory(outSkin, &scratchSkin, sizeof(vskin));
  return TRUE;
}

void free_vskin(vskin* skin){
  vfree(skin->joints, skin->jointCount*sizeof(vjoint), MEMORY_TAG_RENDERER);
}
