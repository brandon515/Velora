#include "core/utils/vgltf.h"
#include "core/utils/vjson.h"
#include "core/utils/vfile.h"
#include "core/memory/vmemory.h"
#include "core/logger.h"
#include "core/utils/vstring.h"
//#define __STDC_NO_THREADS__
#ifndef __STDC_NO_THREADS__
#include <threads.h>
#endif

static const char * EXTENSIONS_SUPPORTED[] = {
  "None",
};
static const u64 EXTENSIONS_SUPPORTED_COUNT = 1;

typedef b8 (*processFuncStr)(json_value*,void*,const char*);
typedef b8 (*processFunc)(json_value*,void*);

b8 extract_gltf_buffer(json_value* buffer, void* out_ptr, const char* uriPath){
  gltf_buffer *out_buffer = (gltf_buffer*)out_ptr;
  if(buffer->type != VELORA_JSON_OBJECT){
    VERROR("Buffer in GLTF File isn't an object");
    return FALSE;
  }
  char *bufferUri = NULL;
  VEL_CHECK(load_json_string(buffer, "uri", &bufferUri));
  if(bufferUri != NULL){
    const char *dataStr = "data:";
    b8 isDataUri = TRUE;
    for(int i = 0; i < vstrlen(dataStr); i++){
      if(bufferUri[i]!=dataStr[i]){
        isDataUri = FALSE;
        break;
      }
    }
    if(isDataUri == TRUE){
      //TODO: Support data URIs
      VWARN("Unsupported data URI in buffer");
      return FALSE;
    }
  }
  VEL_CHECK(load_json_unsigned_integer(buffer, "byteLength", &out_buffer->size));
  // Doesn't matter if this is false, the name is optional;
  out_buffer->name = NULL;
  load_json_string(buffer, "name", &out_buffer->name);
  out_buffer->buffer = vallocate(out_buffer->size, MEMORY_TAG_GLTF);
  char * fullUri = vconcat(uriPath, bufferUri);
  vfree(bufferUri, vstrlen(bufferUri)+1, MEMORY_TAG_STRING);
  velora_file bufferContents = {0};
  VEL_CHECK_MSG(get_file_contents(fullUri, &bufferContents), "Unable to get contents of buffer file with URI %s", fullUri);
  vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
  vcopy_memory(out_buffer->buffer, bufferContents.contents, out_buffer->size);
  free_velora_file(&bufferContents);
  return TRUE;
}

b8 extract_gltf_buffer_view(json_value* buffer_view, void* out_ptr){
  gltf_buffer_view* out_view = (gltf_buffer_view*)out_ptr;
  // These two are required, return FALSE if they're not here
  VEL_CHECK_MSG(load_json_unsigned_integer(buffer_view, "buffer", &out_view->bufferIndex), "No buffer referenced in buffer view");
  VEL_CHECK_MSG(load_json_unsigned_integer(buffer_view, "byteLength", &out_view->size), "No byteLength in buffer view");
  // Offset isn't required but has a default value of 0
  out_view->offset = 0;
  load_json_unsigned_integer(buffer_view, "byteOffset", &out_view->offset);
  // Type isn't require but there's no acceptable value of 0 so this is sufficient for checking
  out_view->type = 0;
  load_json_unsigned_integer(buffer_view, "target", &out_view->type);
  // Stride isn't required, there is no default value in the spec but a value of 0 should be seamless
  out_view->stride = 0;
  load_json_unsigned_integer(buffer_view, "byteStride", &out_view->stride);
  // Name isn't required so it is set to NULL if it doesn't exist
  out_view->name = NULL;
  load_json_string(buffer_view, "name", &out_view->name);
  return TRUE;
}

b8 extract_gltf_sparse_accessor(json_value* sparseAccessor, gltf_sparse_accessor* out_acc){
  if(sparseAccessor->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  json_value indices = {0};
  json_value values = {0};
  VEL_CHECK_MSG(load_json_object(sparseAccessor, "indices", &indices), "No indicies variable in sparse accessor");
  VEL_CHECK_MSG(load_json_object(sparseAccessor, "values", &values), "No values variable in sparse accessor");
  VEL_CHECK_MSG(load_json_unsigned_integer(sparseAccessor, "count", &out_acc->count), "No count variable in sparse accessor");
  
  //Load the indicies
  VEL_CHECK_MSG(load_json_unsigned_integer(&indices, "bufferView", &out_acc->indices.bufferViewIndex), "Indices object in sparse accessor doesn't have a buffer view reference");
  VEL_CHECK_MSG(load_json_unsigned_integer(&indices, "componentType", &out_acc->indices.cType), "Indices object in sparse accessor doesn't have a component type");
  out_acc->indices.byteOffset = 0;
  load_json_unsigned_integer(&indices, "byteOffset", &out_acc->indices.byteOffset);

  //Load the values
  VEL_CHECK_MSG(load_json_unsigned_integer(&values, "bufferView", &out_acc->values.bufferViewIndex), "Values object in sparse accessor doesn't have a buffer view reference");
  out_acc->values.byteOffset = 0;
  load_json_unsigned_integer(&values, "byteOffset", &out_acc->values.byteOffset);

  return TRUE;
}

b8 extract_gltf_value_array(json_value* accessor, const char* variable, gltf_value **outArr, u64 *count, u64 cType){
  if(cType == GLTF_FLOAT){
    f64 *valueArray = NULL;
    if(load_json_float_array(accessor, variable, &valueArray, count) == TRUE){
      (*outArr) = vallocate(sizeof(gltf_value)*(*count), MEMORY_TAG_GLTF);
      for(int i = 0; i < (*count); i++){
        (*outArr)[i].dFloat = valueArray[i];
      }
      vfree(valueArray, sizeof(f64)*(*count), MEMORY_TAG_JSON);
    }else{
      return FALSE;
    }
  }else if(cType%2 == 0){ // Signed
    i64 *valueArray = NULL;
    if(load_json_signed_integer_array(accessor, variable, &valueArray, count) == TRUE){
      (*outArr) = vallocate(sizeof(gltf_value)*(*count), MEMORY_TAG_GLTF);
      for(int i = 0; i < (*count); i++){
        (*outArr)[i].signedInteger = valueArray[i];
      }
      vfree(valueArray, sizeof(i64)*(*count), MEMORY_TAG_JSON);
    }else{
      return FALSE;
    }
  }else if(cType%2 == 1){ // Unsigned
    u64 *valueArray = NULL;
    if(load_json_unsigned_integer_array(accessor, variable, &valueArray, count) == TRUE){
      (*outArr) = vallocate(sizeof(gltf_value)*(*count), MEMORY_TAG_GLTF);
      for(int i = 0; i < (*count); i++){
        (*outArr)[i].unsignedInteger = valueArray[i];
      }
      vfree(valueArray, sizeof(u64)*(*count), MEMORY_TAG_JSON);
    }else{
      return FALSE;
    }
  }
  return TRUE;
}

b8 extract_gltf_accessor(json_value* accessor, void *out_ptr){
  gltf_accessor* out_acc = (gltf_accessor*)out_ptr;
  json_value sparseObj = {0};
  b8 sparseAccessorExists = load_json_object(accessor, "sparse", &sparseObj);
  out_acc->bufferViewIndex = U64_MAX;
  load_json_unsigned_integer(accessor, "bufferView", &out_acc->bufferViewIndex);
  out_acc->offset = 0;
  load_json_unsigned_integer(accessor, "byteOffset", &out_acc->offset);
  VEL_CHECK_MSG(load_json_unsigned_integer(accessor, "componentType", &out_acc->componentType), "Accessor had malformed or non-existent componentType variable");
  u64 normalizedBool = FALSE;
  load_json_unsigned_integer(accessor, "normalized", &normalizedBool);
  out_acc->normalized = normalizedBool;
  VEL_CHECK_MSG(load_json_unsigned_integer(accessor, "count", &out_acc->count), "Accessor had a malformed or non-existent variable");
  VEL_CHECK_MSG(load_json_string(accessor, "type", &out_acc->type), "Accessor had a malformed or non-existent variable");
  out_acc->max = NULL;
  extract_gltf_value_array(accessor, "max", &out_acc->max, &out_acc->max_count, out_acc->componentType);
  out_acc->min = NULL;
  extract_gltf_value_array(accessor, "min", &out_acc->min, &out_acc->min_count, out_acc->componentType);
  out_acc->name = NULL;
  load_json_string(accessor, "name", &out_acc->name);
  if(sparseAccessorExists == TRUE){
    VEL_CHECK(extract_gltf_sparse_accessor(&sparseObj, &out_acc->sparse));
  }else{
    out_acc->sparse.count = 0;
  }
  return TRUE;
}

b8 extract_gltf_image(json_value* image, void *out_ptr, const char* uriPath){
  gltf_image *out_image = (gltf_image*)out_ptr;
  char *uri = NULL;
  if(load_json_string(image, "uri", &uri) == TRUE){
    char *fullUri = vconcat(uriPath, uri);
    VEL_CHECK_MSG(import_pixels(fullUri, &out_image->uriData), "Unable to import pixel data from %s", fullUri);
    vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
    vfree(uri, vstrlen(uri)+1, MEMORY_TAG_STRING);
    // This value is optionally definied when URI is defined
    out_image->mimeType = NULL;
    load_json_string(image, "mimeType", &out_image->mimeType);
    out_image->bufferViewIndex = U64_MAX;
  }else{
    // However, it has to be defined with buffer view
    VEL_CHECK_MSG(load_json_string(image, "mimeType", &out_image->mimeType), "Unable to process mimeType in GLTF Image");
    VEL_CHECK_MSG(load_json_unsigned_integer(image, "bufferView", &out_image->bufferViewIndex), "Unable to process bufferView in GLTF Image");
    out_image->uriData.size = 0;
  }
  out_image->name = NULL;
  load_json_string(image, "name", &out_image->name);
  return TRUE;
}

b8 extract_gltf_texture(json_value* texture, void *out_ptr){
  gltf_texture *out_texture = (gltf_texture*)out_ptr;
  out_texture->sampler = U64_MAX;
  load_json_unsigned_integer(texture, "sampler", &out_texture->sampler);
  json_value extension = {0};
  b8 extensionExists = load_json_object(texture, "extensions", &extension);
  out_texture->source = U64_MAX;
  if(
    load_json_unsigned_integer(texture, "source", &out_texture->source) == FALSE &&
    extensionExists == FALSE
  ){
    VWARN("Texture source is undefined and no extensions were found. GLTF is malformed");
    return FALSE;
  }else if(extensionExists == TRUE){
    VWARN("Texture is using extension, this is currently not supported by Velora");
  }
  out_texture->name = NULL;
  load_json_string(texture, "name", &out_texture->name);
  return TRUE;
}

b8 extract_texture_info(json_value* textureInfo, gltf_texture_info *out_texture_info){
  // This is the only required value
  VEL_CHECK(load_json_unsigned_integer(textureInfo, "index", &out_texture_info->textureIndex));
  out_texture_info->texCoordIndex = 0;
  load_json_unsigned_integer(textureInfo, "texCoord", &out_texture_info->texCoordIndex);
  out_texture_info->scaleStrength = 1;
  load_json_float(textureInfo, "scale", &out_texture_info->scaleStrength);
  load_json_float(textureInfo, "strength", &out_texture_info->scaleStrength);
  return TRUE;
}

// This is weird actually, this method can't fail which means theortically an empty material is valid
b8 extract_metallic_roughness(json_value* roughness, gltf_metal_roughness *out_roughness){
  out_roughness->baseColor = (vec4){{1,1,1,1}};
  f64 *baseColorFactor = NULL;
  u64 factorCount = 0;
  if(load_json_float_array(roughness, "baseColorFactor", &baseColorFactor, &factorCount) == TRUE && factorCount <= 4){
    for(int i = 0; i < factorCount; i++){
      out_roughness->baseColor.xyzw[i] = baseColorFactor[i];
    }
    vfree(baseColorFactor, sizeof(f64)*factorCount, MEMORY_TAG_JSON);
  }
  json_value baseColorTexture = {0};
  if(load_json_object(roughness, "baseColorTexture", &baseColorTexture) == TRUE){
    VEL_CHECK_MSG(extract_texture_info(&baseColorTexture, &out_roughness->baseColorTexture), "Malformed baseColorTexture in material");
  }else{
    out_roughness->baseColorTexture.textureIndex = U64_MAX;
  }
  out_roughness->metallicFactor = 1.0f;
  load_json_float(roughness, "metallicFactor", &out_roughness->metallicFactor);
  out_roughness->roughnessFactor = 1.0f;
  load_json_float(roughness, "roughnessFactor", &out_roughness->roughnessFactor);
  json_value metallicRoughnessTexture = {0};
  if(load_json_object(roughness, "metallicRoughnessTexture", &metallicRoughnessTexture) == TRUE){
    VEL_CHECK_MSG(extract_texture_info(&metallicRoughnessTexture, &out_roughness->metallicRoughnessTexture), "Malformed metallicRoughnessTexture in material");
  }else{
    out_roughness->metallicRoughnessTexture.textureIndex = U64_MAX;
  }
  return TRUE;
}

b8 extract_gltf_material(json_value* material, void *out_ptr){
  gltf_material *out_material = (gltf_material*)out_ptr;
  out_material->name = NULL;
  load_json_string(material, "name", &out_material->name);
  json_value pbrMetalRoughness = {0};
  if(load_json_object(material, "pbrMetallicRoughness", &pbrMetalRoughness) == TRUE){
    VEL_CHECK_MSG(extract_metallic_roughness(&pbrMetalRoughness, &out_material->pbrMetallicRoughness), "Malformed pbrMetallicRoughness in Material");
  }else{ // Load the default values as outlined in the spec
    out_material->pbrMetallicRoughness.baseColor = (vec4){{1,1,1,1}};
    out_material->pbrMetallicRoughness.metallicFactor = 1.0f;
    out_material->pbrMetallicRoughness.roughnessFactor = 1.0f;
    out_material->pbrMetallicRoughness.baseColorTexture.textureIndex = U64_MAX;
    out_material->pbrMetallicRoughness.metallicRoughnessTexture.textureIndex = U64_MAX;
  }
  json_value normalTexture = {0};
  if(load_json_object(material, "normalTexture", &normalTexture) == TRUE){
    extract_texture_info(&normalTexture, &out_material->normalTexture);
  }else{
    out_material->normalTexture.textureIndex = U64_MAX;
  }
  json_value occlusionTexture = {0};
  if(load_json_object(material, "occlusionTexture", &occlusionTexture) == TRUE){
    extract_texture_info(&occlusionTexture, &out_material->occlusionTexture);
  }else{
    out_material->occlusionTexture.textureIndex = U64_MAX;
  }
  json_value emissiveTexture = {0};
  if(load_json_object(material, "emissiveTexture", &emissiveTexture) == TRUE){
    extract_texture_info(&emissiveTexture, &out_material->emissiveTexture);
  }else{
    out_material->emissiveTexture.textureIndex = U64_MAX;
  }
  out_material->emissiveFactor = (vec3){{0,0,0}};
  f64 *emissiveArray = NULL;
  u64 emissiveCount = 0;
  if(load_json_float_array(material, "emissiveFactor", &emissiveArray, &emissiveCount) == TRUE && emissiveCount <= 3){
    for(int i = 0; i < emissiveCount; i++){
      out_material->emissiveFactor.xyz[i] = emissiveArray[i];
    }
    vfree(emissiveArray, sizeof(f64)*emissiveCount, MEMORY_TAG_JSON);
  }
  if(load_json_string(material, "alphaMode", &out_material->alphaMode) == FALSE){
    const char *defaultAlphaMode = "OPAQUE";
    out_material->alphaMode = vallocate(vstrlen(defaultAlphaMode)+1, MEMORY_TAG_STRING);
    vcopy_memory(out_material->alphaMode, defaultAlphaMode, vstrlen(defaultAlphaMode)+1);
  }
  out_material->alphaCutoff = 0.5f;
  load_json_float(material, "alphaCutoff", &out_material->alphaCutoff);
  u64 doubleSidedTrueFalse = FALSE;
  load_json_unsigned_integer(material, "doubleSided", &doubleSidedTrueFalse);
  out_material->doubleSided = doubleSidedTrueFalse;
  return TRUE;
}

b8 extract_gltf_mesh_primitive_attribute(json_value* attributes, gltf_mesh_primitive_attribute *out_attributes){
  out_attributes->position = U64_MAX;
  load_json_unsigned_integer(attributes, "POSITION", &out_attributes->position);
  out_attributes->normal = U64_MAX;
  load_json_unsigned_integer(attributes, "NORMAL", &out_attributes->normal);
  out_attributes->tangent = U64_MAX;
  load_json_unsigned_integer(attributes, "TANGENT", &out_attributes->tangent);
  const u64 MAX_ATT_NUMBER = 10;
  u64 texcoord[MAX_ATT_NUMBER];
  u64 color[MAX_ATT_NUMBER];
  u64 joints[MAX_ATT_NUMBER];
  u64 weights[MAX_ATT_NUMBER];
  out_attributes->texCount = 0;
  out_attributes->colorCount = 0;
  out_attributes->jointCount = 0;
  out_attributes->weightCount = 0;
  char texName[] = "TEXCOORD_n";
  char colorName[] = "COLOR_n";
  char jointName[] = "JOINTS_n";
  char weightName[] = "WEIGHTS_n";
  for(int i = 0; i < MAX_ATT_NUMBER; i++){
    char textNumber = '0'+i;
    texName[9] = textNumber;// position 9 is the n in the string
    if(load_json_unsigned_integer(attributes, texName, &texcoord[i]) == TRUE){
      out_attributes->texCount++;
    }
    colorName[6] = textNumber;
    if(load_json_unsigned_integer(attributes, colorName, &color[i]) == TRUE){
      out_attributes->colorCount++;
    }
    jointName[7] = textNumber;
    if(load_json_unsigned_integer(attributes, jointName, &joints[i]) == TRUE){
      out_attributes->jointCount++;
    }
    weightName[8] = textNumber;
    if(load_json_unsigned_integer(attributes, weightName, &weights[i]) == TRUE){
      out_attributes->weightCount++;
    }
  }
  if(out_attributes->texCount != 0){
    out_attributes->texCoords = vallocate(out_attributes->texCount*sizeof(u64), MEMORY_TAG_GLTF);
  }
  if(out_attributes->colorCount != 0){
    out_attributes->colors = vallocate(out_attributes->colorCount*sizeof(u64), MEMORY_TAG_GLTF);
  }
  if(out_attributes->jointCount != 0){
    out_attributes->joints = vallocate(out_attributes->jointCount*sizeof(u64), MEMORY_TAG_GLTF);
  }
  if(out_attributes->weightCount != 0){
    out_attributes->weights = vallocate(out_attributes->weightCount*sizeof(u64), MEMORY_TAG_GLTF);
  }
  for(int i = 0; i < MAX_ATT_NUMBER; i++){
    if(i < out_attributes->texCount){
      out_attributes->texCoords[i] = texcoord[i];
    }
    if(i < out_attributes->colorCount){
      out_attributes->colors[i] = color[i];
    }
    if(i < out_attributes->jointCount){
      out_attributes->joints[i] = joints[i];
    }
    if(i < out_attributes->weightCount){
      out_attributes->weights[i] = weights[i];
    }
  }
  return TRUE;
}

b8 extract_gltf_mesh_primitive(json_value* primitive, gltf_mesh_primitive *out_primitive){
  json_value attributes = {0};
  VEL_CHECK_MSG(load_json_object(primitive, "attributes", &attributes), "No attributes variable in mesh primitive");
  VEL_CHECK(extract_gltf_mesh_primitive_attribute(&attributes, &out_primitive->attributes));
  out_primitive->indicies = U64_MAX;
  load_json_unsigned_integer(primitive, "indices", &out_primitive->indicies);
  out_primitive->material = U64_MAX;
  load_json_unsigned_integer(primitive, "material", &out_primitive->material);
  out_primitive->mode = 4;
  load_json_unsigned_integer(primitive, "mode", &out_primitive->mode);
  json_value *morphTargets = NULL;
  if(load_json_object_array(primitive, "targets", &morphTargets, &out_primitive->morphCount) == TRUE){
    out_primitive->morphTargets = vallocate(sizeof(gltf_mesh_primitive_attribute)*out_primitive->morphCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_primitive->morphCount; i++){
      extract_gltf_mesh_primitive_attribute(&morphTargets[i], &out_primitive->morphTargets[i]);
    }
    //vfree(morphTargets, sizeof(json_value)*out_primitive->morphCount, MEMORY_TAG_JSON);
  }
  return TRUE;
}

b8 extract_gltf_mesh(json_value *mesh, void *out_ptr){
  gltf_mesh *out_mesh = (gltf_mesh*)out_ptr;
  json_value *meshes = NULL;
  VEL_CHECK_MSG(load_json_object_array(mesh, "primitives", &meshes, &out_mesh->primitiveCount), "No primitives attribute in mesh");
  out_mesh->primitives = vallocate(sizeof(gltf_mesh_primitive)*out_mesh->primitiveCount, MEMORY_TAG_GLTF);
  for(int i = 0; i < out_mesh->primitiveCount; i++){
    VEL_CHECK_MSG(extract_gltf_mesh_primitive(&meshes[i], &out_mesh->primitives[i]), "Malformed primitive in mesh");
  }
  //vfree(meshes, sizeof(json_value)*out_mesh->primitiveCount, MEMORY_TAG_JSON);
  
  load_json_float_array(mesh, "weights", &out_mesh->weights, &out_mesh->weightCount);
  load_json_string(mesh, "name", &out_mesh->name);
  return TRUE;
}

b8 extract_gltf_animation_sampler(json_value *sampler, gltf_animation_sampler *out_sampler){
  VEL_CHECK_MSG(load_json_unsigned_integer(sampler, "input", &out_sampler->input), "No input variable in animation sampler");
  VEL_CHECK_MSG(load_json_unsigned_integer(sampler, "output", &out_sampler->output), "No output variable in animation sampler");
  if(load_json_string(sampler, "interpolation", &out_sampler->interpolation) == FALSE){
    const char defaultInt[] = "LINEAR";
    out_sampler->interpolation = vallocate(vstrlen(defaultInt)+1, MEMORY_TAG_STRING);
    vcopy_memory(out_sampler->interpolation, defaultInt, vstrlen(defaultInt));
  }
  return TRUE;
}

b8 extract_gltf_animation_channel(json_value *channel, gltf_animation_channel *out_channel){
  VEL_CHECK_MSG(load_json_unsigned_integer(channel, "sampler", &out_channel->sampler), "No sampler variable in animation channel");
  json_value target = {0};
  VEL_CHECK_MSG(load_json_object(channel, "target", &target), "No target variable in animation channel");
  out_channel->target.node = U64_MAX;
  load_json_unsigned_integer(&target, "node", &out_channel->target.node);
  VEL_CHECK_MSG(load_json_string(&target, "path", &out_channel->target.path), "No path variable in animation channel target");
  return TRUE;
}

b8 extract_gltf_animation(json_value *animation, void *out_ptr){
  gltf_animation *out_animation = (gltf_animation*)out_ptr;
  json_value *channels = NULL;
  VEL_CHECK_MSG(load_json_object_array(animation, "channels", &channels, &out_animation->channelCount), "No channels variable in GLTF animation");
  out_animation->channels = vallocate(sizeof(gltf_animation_channel)*out_animation->channelCount, MEMORY_TAG_GLTF);
  for(int i = 0; i < out_animation->channelCount; i++){
    VEL_CHECK(extract_gltf_animation_channel(&channels[i], &out_animation->channels[i]));
  }
  //vfree(channels, sizeof(json_value)*out_animation->channelCount, MEMORY_TAG_JSON);

  json_value *samplers = NULL;
  VEL_CHECK_MSG(load_json_object_array(animation, "samplers", &samplers, &out_animation->samplerCount), "No samplers variable in GLTF animation");
  out_animation->samplers = vallocate(sizeof(gltf_animation_sampler)*out_animation->samplerCount, MEMORY_TAG_GLTF);
  for(int i = 0; i < out_animation->samplerCount; i++){
    VEL_CHECK(extract_gltf_animation_sampler(&samplers[i], &out_animation->samplers[i]));
  }
  //vfree(samplers, sizeof(json_value)*out_animation->samplerCount, MEMORY_TAG_JSON);
  out_animation->name = NULL;
  load_json_string(animation, "name", &out_animation->name);
  return TRUE;
}

b8 extract_gltf_camera_perspective(json_value *perspective, gltf_camera_perspective *out_per){
  out_per->aspectRatio = 0;
  load_json_float(perspective, "aspectRatio", &out_per->aspectRatio);
  VEL_CHECK_MSG(load_json_float(perspective, "yfov", &out_per->yfov), "Variable yfov not in GLTF camera perspective");
  out_per->zfar = 0;
  load_json_float(perspective, "zfar", &out_per->zfar);
  VEL_CHECK_MSG(load_json_float(perspective, "znear", &out_per->znear), "Variable znear not in GLTF camera perspective");
  return TRUE;
}

b8 extract_gltf_camera_orthographic(json_value *orthographic, gltf_camera_orthographic *out_ortho){
  VEL_CHECK_MSG(load_json_float(orthographic, "xmag", &out_ortho->xmag), "Variable xmag not in GLTF camera orthographic");
  VEL_CHECK_MSG(load_json_float(orthographic, "ymag", &out_ortho->ymag), "Variable ymag not in GLTF camera orthographic");
  VEL_CHECK_MSG(load_json_float(orthographic, "zfar", &out_ortho->zfar), "Variable zfar not in GLTF camera orthographic");
  VEL_CHECK_MSG(load_json_float(orthographic, "znear", &out_ortho->znear), "Variable znear not in GLTF camera orthographic");
  return TRUE;
}

b8 extract_gltf_camera(json_value *camera, void *out_ptr){
  gltf_camera *out_camera = (gltf_camera*)out_ptr;
  VEL_CHECK_MSG(load_json_string(camera, "type", &out_camera->type), "Camera type not identified");
  json_value cameraData = {0};
  if(vstrcmp(out_camera->type, "perspective") == TRUE){
    //TODO: fix json code, if a string is the same name as a variable than it won't load
    VEL_CHECK_MSG(load_json_object(camera, "perspective", &cameraData), "Perspective camera data not present while type was identified as perspective");
    extract_gltf_camera_perspective(&cameraData, &out_camera->perspective);
  }else if(vstrcmp(out_camera->type, "orthographic") == TRUE){
    VEL_CHECK_MSG(load_json_object(camera, "orthographic", &cameraData), "Orthographic camera data not present while type was identified as orthographic");
    extract_gltf_camera_orthographic(&cameraData, &out_camera->orthographic);
  }else{
    VERROR("GLTF Camera had illegal type %s", out_camera->type);
    vfree(out_camera->type, vstrlen(out_camera->type)+1, MEMORY_TAG_STRING);
    return FALSE;
  }
  load_json_string(camera, "name", &out_camera->name);
  return TRUE;
}

b8 extract_gltf_skin(json_value *skin, void *out_ptr){
  gltf_skin *out_skin = (gltf_skin*)out_ptr;
  out_skin->inverseBindMatrices = U64_MAX;
  load_json_unsigned_integer(skin, "inverseBindMatrices", &out_skin->inverseBindMatrices);
  out_skin->skeleton = U64_MAX;
  load_json_unsigned_integer(skin, "skeleton", &out_skin->skeleton);
  VEL_CHECK(load_json_unsigned_integer_array(skin, "joints", &out_skin->joints, &out_skin->jointCount));
  out_skin->name = NULL;
  load_json_string(skin, "name", &out_skin->name);
  return TRUE;
}

b8 extract_gltf_asset(json_value *asset, gltf_asset *out_asset){
  out_asset->copyright = NULL;
  load_json_string(asset, "copyright", &out_asset->copyright);
  out_asset->generator = NULL;
  load_json_string(asset, "generator", &out_asset->generator);
  out_asset->minVersion = NULL;
  load_json_string(asset, "minVersion", &out_asset->minVersion);
  VEL_CHECK(load_json_string(asset, "version", &out_asset->version));
  return TRUE;
}

b8 extract_gltf_sampler(json_value *sampler, void *out_ptr){
  gltf_sampler *out_sampler = (gltf_sampler*)out_ptr;
  out_sampler->magFilter = U64_MAX;
  load_json_unsigned_integer(sampler, "magFilter", &out_sampler->magFilter);
  out_sampler->minFilter = U64_MAX;
  load_json_unsigned_integer(sampler, "minFilter", &out_sampler->minFilter);
  out_sampler->wrapS = GLTF_SAMPLER_WRAP_REPEAT;
  load_json_unsigned_integer(sampler, "wrapS", &out_sampler->wrapS);
  out_sampler->wrapT = GLTF_SAMPLER_WRAP_REPEAT;
  load_json_unsigned_integer(sampler, "wrapT", &out_sampler->wrapT);
  out_sampler->name = NULL;
  load_json_string(sampler, "name", &out_sampler->name);
  return TRUE;
}

b8 extract_gltf_node(json_value *node, void *out_ptr){
  gltf_node *out_node = (gltf_node*)out_ptr;
  out_node->camera = U64_MAX;
  load_json_unsigned_integer(node, "camera", &out_node->camera);
  out_node->children = NULL;
  out_node->childCount = 0;
  load_json_unsigned_integer_array(node, "children", &out_node->children, &out_node->childCount);
  out_node->skin = U64_MAX;
  load_json_unsigned_integer(node, "skin", &out_node->skin);
  out_node->matrix = indentity_mat4();
  f64 *floatArray = NULL;
  u64 floatCount = 0;
  if(load_json_float_array(node, "matrix", &floatArray, &floatCount) == TRUE){
    if(floatCount != 16){
      VERROR("Matrix in GLTF Node doesn't have 16 floats");
      return FALSE;
    }
    for(int i = 0; i < floatCount; i++){
      out_node->matrix.mat[i] = floatArray[i];
    }
    vfree(floatArray, sizeof(f64)*floatCount, MEMORY_TAG_JSON);
    floatArray = NULL;
    floatCount = 0;
  }
  out_node->mesh = U64_MAX;
  load_json_unsigned_integer(node, "mesh", &out_node->mesh);
  out_node->rotation.x = 0;
  out_node->rotation.y = 0;
  out_node->rotation.z = 0;
  out_node->rotation.w = 1;
  if(load_json_float_array(node, "rotation", &floatArray, &floatCount) == TRUE){
    if(floatCount != 4){
      VERROR("Rotations quat in GLTF Node doesn't have 4 floats");
      return FALSE;
    }
    for(int i = 0; i < floatCount; i++){
      out_node->rotation.xyzw[i] = floatArray[i];
    }
    vfree(floatArray, sizeof(f64)*floatCount, MEMORY_TAG_JSON);
    floatArray = NULL;
    floatCount = 0;
  }
  out_node->scale.x = 1;
  out_node->scale.y = 1;
  out_node->scale.z = 1;
  if(load_json_float_array(node, "scale", &floatArray, &floatCount) == TRUE){
    if(floatCount != 3){
      VERROR("Scale in GLTF Node doesn't have 3 floats");
      return FALSE;
    }
    for(int i = 0; i < floatCount; i++){
      out_node->scale.xyz[i] = floatArray[i];
    }
    vfree(floatArray, sizeof(f64)*floatCount, MEMORY_TAG_JSON);
    floatArray = NULL;
    floatCount = 0;
  }
  out_node->translation.x = 0;
  out_node->translation.y = 0;
  out_node->translation.z = 0;
  if(load_json_float_array(node, "translation", &floatArray, &floatCount) == TRUE){
    if(floatCount != 3){
      VERROR("Translation in GLTF Node doesn't have 3 floats");
      return FALSE;
    }
    for(int i = 0; i < floatCount; i++){
      out_node->translation.xyz[i] = floatArray[i];
    }
    vfree(floatArray, sizeof(f64)*floatCount, MEMORY_TAG_JSON);
    floatArray = NULL;
    floatCount = 0;
  }
  out_node->weights = NULL;
  out_node->weightCount = 0;
  load_json_float_array(node, "weights", &out_node->weights, &out_node->weightCount);
  out_node->name = NULL;
  load_json_string(node, "name", &out_node->name);
  return TRUE;
}

b8 extract_gltf_scene(json_value *scene, void *out_ptr){
  gltf_scene *out_scene = (gltf_scene*)out_ptr;
  out_scene->nodes = NULL;
  out_scene->nodeCount = 0;
  load_json_unsigned_integer_array(scene, "nodes", &out_scene->nodes, &out_scene->nodeCount);
  out_scene->name = NULL;
  load_json_string(scene, "name", &out_scene->name);
  return TRUE;
}

#ifndef __STDC_NO_THREADS__
//json_value* buffer, gltf_buffer* out_buffer, const char* uriPath
typedef struct _thread_data{
  json_value *in;
  void *out;
  b8 (*processFuncStr)(json_value*,void*,const char*);
  b8 (*processFunc)(json_value*,void*);
  const char *str;
}thread_data;
int import_thread(void* data){
  thread_data *bufData = (thread_data*)data;
  if(bufData->str != NULL){
    VEL_CHECK(bufData->processFuncStr(bufData->in, bufData->out, bufData->str));
  }else{
    VEL_CHECK(bufData->processFunc(bufData->in, bufData->out));
  }
  vfree(data, sizeof(thread_data), MEMORY_TAG_GLTF);
  return TRUE;
}
#endif

b8 check_extension_validity(char **extensionList, u64 extensionCount){
  b8 listValid = TRUE;
  for(int i = 0; i < extensionCount; i++){
    b8 extensionValid = FALSE;
    for(int j = 0; j < EXTENSIONS_SUPPORTED_COUNT; j++){
      if(vstrcmp(EXTENSIONS_SUPPORTED[j], extensionList[i]) == TRUE){
        extensionValid = TRUE;
        break;
      }
    }
    if(extensionValid == FALSE){
      VWARN("Unsupported extension: %s",extensionList[i]);
      listValid = FALSE;
    }
  }
  return listValid;
}

thread_data* fill_thread_str_data(json_value *in, void *out, processFuncStr func, const char* str){
  thread_data *ret_dat = vallocate(sizeof(thread_data), MEMORY_TAG_GLTF);
  ret_dat->in = in;
  ret_dat->out = out;
  ret_dat->processFunc = NULL;
  ret_dat->processFuncStr = func;
  ret_dat->str = str;
  return ret_dat;
}

thread_data* fill_thread_data(json_value *in, void *out, processFunc func){
  thread_data *ret_dat = vallocate(sizeof(thread_data), MEMORY_TAG_GLTF);
  ret_dat->in = in;
  ret_dat->out = out;
  ret_dat->processFunc = func;
  ret_dat->processFuncStr = NULL;
  ret_dat->str = NULL;
  return ret_dat;
}

void start_import_str_thread(u64 objStride, u64 *objCount, void** out, const char *jsonName, processFuncStr func, json_value *gltfJson, const char* uriPath, thrd_t *threadIds, u64 *threadCount){
  json_value *jsonObj = NULL;
  if(load_json_object_array(gltfJson, jsonName, &jsonObj, objCount) == TRUE){
    (*out) = vallocate(objStride*(*objCount), MEMORY_TAG_GLTF);
    for(int i = 0; i < (*objCount); i++){
      thread_data *threadData = fill_thread_str_data(&jsonObj[i], (*out)+(objStride*i), func, uriPath);
      thrd_create(threadIds+(*threadCount), import_thread, threadData);
      (*threadCount)++;
    }
  }
}

void start_import_thread(u64 objStride, u64 *objCount, void** out, const char *jsonName, processFunc func, json_value *gltfJson, thrd_t *threadIds, u64 *threadCount){
  json_value *jsonObj = NULL;
  if(load_json_object_array(gltfJson, jsonName, &jsonObj, objCount) == TRUE){
    (*out) = vallocate(objStride*(*objCount), MEMORY_TAG_GLTF);
    for(int i = 0; i < (*objCount); i++){
      thread_data *threadData = fill_thread_data(&jsonObj[i], (*out)+(objStride*i), func);
      thrd_create(threadIds+(*threadCount), import_thread, threadData);
      (*threadCount)++;
    }
  }
}

b8 import_gltf(const char *uri, gltf_object *out_gltf){
  #ifdef __STDC_NO_THREADS__
  VFATAL("Compiler doesn't support C11 threads, this is needed to import GLTF Files");
  return FALSE;
  #endif
  json_value gltfJson = {0};
  if(import_json_file(uri, &gltfJson) == FALSE){
    VERROR("Unable to read GLTF file %s", uri);
    return FALSE;
  }

  thrd_t threadIds[512];
  u64 threadCount = 0;
  char* uriPath = get_file_path(uri);

  json_value jsonValue = {0};
  load_json_object(&gltfJson, "asset", &jsonValue);
  VEL_CHECK(extract_gltf_asset(&jsonValue, &out_gltf->asset));
  out_gltf->scene = U64_MAX;
  load_json_unsigned_integer(&gltfJson, "scene", &out_gltf->scene);

  char **extensionsRequired = NULL;
  u64 extentionCount = 0;
  if(load_json_string_array(&gltfJson, "extensionsRequired", &extensionsRequired, &extentionCount) == TRUE){
    if(check_extension_validity(extensionsRequired, extentionCount) == FALSE){
      VERROR("GLTF File %s requires unsupported extension(s), unable to be imported", uri);
      return FALSE;
    }
    for(int i = 0; i < extentionCount; i++){
      vfree(extensionsRequired[i], vstrlen(extensionsRequired[i])+1, MEMORY_TAG_STRING);
    }
    vfree(extensionsRequired, sizeof(char*)*extentionCount, MEMORY_TAG_JSON);
  }

  char **extensionsUsed = NULL;
  extentionCount = 0;
  if(load_json_string_array(&gltfJson, "extensionsUsed", &extensionsUsed, &extentionCount) == TRUE){
    if(check_extension_validity(extensionsUsed, extentionCount) == FALSE){
      VWARN("GLTF File %s uses unsupported extension(s), they are not required so import will continue", uri);
    }
    for(int i = 0; i < extentionCount; i++){
      vfree(extensionsUsed[i], vstrlen(extensionsUsed[i])+1, MEMORY_TAG_STRING);
    }
    vfree(extensionsUsed, sizeof(char*)*extentionCount, MEMORY_TAG_JSON);
  }

  start_import_str_thread(
    sizeof(gltf_buffer), 
    &out_gltf->bufferCount, 
    (void**)&out_gltf->buffers, 
    "buffers", 
    extract_gltf_buffer,
    &gltfJson,
    uriPath,
    threadIds,
    &threadCount
  );

  start_import_thread(
    sizeof(gltf_buffer_view), 
    &out_gltf->bufferViewCount, 
    (void**)&out_gltf->bufferViews, 
    "bufferViews", 
    extract_gltf_buffer_view, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_accessor), 
    &out_gltf->accessorCount, 
    (void**)&out_gltf->accessors, 
    "accessors", 
    extract_gltf_accessor, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_str_thread(
    sizeof(gltf_image), 
    &out_gltf->imageCount, 
    (void**)&out_gltf->images, 
    "images", 
    extract_gltf_image, 
    &gltfJson,
    uriPath,
    threadIds,
    &threadCount
  );

  start_import_thread(
    sizeof(gltf_texture), 
    &out_gltf->textureCount, 
    (void**)&out_gltf->textures, 
    "textures", 
    extract_gltf_texture, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_material), 
    &out_gltf->materialCount, 
    (void**)&out_gltf->materials, 
    "materials", 
    extract_gltf_material, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_mesh), 
    &out_gltf->meshCount, 
    (void**)&out_gltf->meshes, 
    "meshes", 
    extract_gltf_mesh, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_animation), 
    &out_gltf->animationCount, 
    (void**)&out_gltf->animations, 
    "animations", 
    extract_gltf_animation, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_camera), 
    &out_gltf->cameraCount, 
    (void**)&out_gltf->cameras, 
    "cameras", 
    extract_gltf_camera, 
    &gltfJson,threadIds, &threadCount
  );
  
  start_import_thread(
    sizeof(gltf_skin), 
    &out_gltf->skinCount, 
    (void**)&out_gltf->skins, 
    "skins", 
    extract_gltf_skin, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_sampler), 
    &out_gltf->samplerCount, 
    (void**)&out_gltf->samplers, 
    "samplers", 
    extract_gltf_sampler, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_node), 
    &out_gltf->nodeCount, 
    (void**)&out_gltf->nodes, 
    "nodes", 
    extract_gltf_node, 
    &gltfJson,threadIds, &threadCount
  );

  start_import_thread(
    sizeof(gltf_scene), 
    &out_gltf->sceneCount, 
    (void**)&out_gltf->scenes, 
    "scenes", 
    extract_gltf_scene, 
    &gltfJson,threadIds, &threadCount
  );

  b8 retVal = TRUE;
  for(int i = 0; i < threadCount; i++){
    int result;
    thrd_join(threadIds[i], &result);
    if(result == FALSE && retVal == TRUE){
      VERROR("Unable to load gltf file %s", uri);
      retVal = FALSE;
    }
  }
  free_json_value(&gltfJson);
  return retVal;
}

void free_gltf_buffer(gltf_buffer* buf){
  vfree(buf->buffer, buf->size, MEMORY_TAG_GLTF);
  if(buf->name != NULL){
    vfree(buf->name, vstrlen(buf->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_buffer_view(gltf_buffer_view* buf){
  if(buf->name != NULL){
    vfree(buf->name, vstrlen(buf->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_accessor(gltf_accessor* acc){
  vfree(acc->type, vstrlen(acc->type)+1, MEMORY_TAG_GLTF);
  if(acc->max_count != 0){
    vfree(acc->max, acc->max_count*sizeof(gltf_value), MEMORY_TAG_GLTF);
  }
  if(acc->min_count != 0){
    vfree(acc->min, acc->min_count*sizeof(gltf_value), MEMORY_TAG_GLTF);
  }
  if(acc->name != NULL){
    vfree(acc->name, vstrlen(acc->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_image(gltf_image* image){
  if(image->name != NULL){
    vfree(image->name, vstrlen(image->name)+1, MEMORY_TAG_STRING);
  }
  if(image->mimeType != NULL){
    vfree(image->mimeType, vstrlen(image->mimeType)+1, MEMORY_TAG_STRING);
  }
  if(image->uriData.size != 0){
    free_pixels(&image->uriData);
  }
}

void free_gltf_texture(gltf_texture* texture){
  if(texture->name != NULL){
    vfree(texture->name, vstrlen(texture->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_material(gltf_material* material){
  if(material->name != NULL){
    vfree(material->name, vstrlen(material->name)+1, MEMORY_TAG_STRING);
  }
  if(material->alphaMode != NULL){
    vfree(material->alphaMode, vstrlen(material->alphaMode)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_mesh_primitive_attribute(gltf_mesh_primitive_attribute *att){
  if(att->texCoords != NULL){
    vfree(att->texCoords, sizeof(u64)*att->texCount, MEMORY_TAG_GLTF);
  }
  if(att->colors != NULL){
    vfree(att->colors, sizeof(u64)*att->colorCount, MEMORY_TAG_GLTF);
  }
  if(att->joints != NULL){
    vfree(att->joints, sizeof(u64)*att->jointCount, MEMORY_TAG_GLTF);
  }
  if(att->weights != NULL){
    vfree(att->weights, sizeof(u64)*att->weightCount, MEMORY_TAG_GLTF);
  }
}

void free_gltf_mesh_primitive(gltf_mesh_primitive *prim){
  free_gltf_mesh_primitive_attribute(&prim->attributes);
  if(prim->morphTargets != NULL){
    for(int i = 0; i < prim->morphCount; i++){
      free_gltf_mesh_primitive_attribute(&prim->morphTargets[i]);
    }
    vfree(prim->morphTargets, sizeof(gltf_mesh_primitive_attribute)*prim->morphCount, MEMORY_TAG_GLTF);
  }
}

void free_gltf_mesh(gltf_mesh *mesh){
  for(int i = 0; i < mesh->primitiveCount; i++){
    free_gltf_mesh_primitive(&mesh->primitives[i]);
  }
  vfree(mesh->primitives, sizeof(gltf_mesh_primitive)*mesh->primitiveCount, MEMORY_TAG_GLTF);
  if(mesh->weights != NULL){
    vfree(mesh->weights, sizeof(f64)*mesh->weightCount, MEMORY_TAG_GLTF);
  }
  if(mesh->name != NULL){
    vfree(mesh->name, vstrlen(mesh->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_animation_sampler(gltf_animation_sampler *out_sampler){
  if(out_sampler->interpolation != NULL){
    vfree(out_sampler->interpolation, vstrlen(out_sampler->interpolation)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_animation_channel_target(gltf_animation_channel_target *out_target){
  if(out_target->path != NULL){
    vfree(out_target->path, vstrlen(out_target->path)+1, MEMORY_TAG_GLTF);
  }
}

void free_gltf_animation_channel(gltf_animation_channel *out_channel){
  free_gltf_animation_channel_target(&out_channel->target);
}

void free_gltf_animation(gltf_animation *out_animation){
  if(out_animation->channels != NULL){
    for(int i = 0; i < out_animation->channelCount; i++){
      free_gltf_animation_channel(&out_animation->channels[i]);
    }
    vfree(out_animation->channels, sizeof(gltf_animation)*out_animation->channelCount, MEMORY_TAG_GLTF);
  }
  if(out_animation->samplers != NULL){
    for(int i = 0; i < out_animation->samplerCount; i++){
      free_gltf_animation_sampler(&out_animation->samplers[i]);
    }
    vfree(out_animation->samplers, sizeof(gltf_animation_sampler)*out_animation->samplerCount, MEMORY_TAG_GLTF);
  }
  if(out_animation->name != NULL){
    vfree(out_animation->name, vstrlen(out_animation->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_camera(gltf_camera *out_camera){
  if(out_camera->type != NULL){
    vfree(out_camera->type, vstrlen(out_camera->type)+1, MEMORY_TAG_STRING);
  }
  if(out_camera->name != NULL){
    vfree(out_camera->name, vstrlen(out_camera->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_skin(gltf_skin *out_skin){
  if(out_skin->joints != NULL){
    vfree(out_skin->joints, out_skin->jointCount*sizeof(u64), MEMORY_TAG_GLTF);
  }
  if(out_skin->name != NULL){
    vfree(out_skin->name, vstrlen(out_skin->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_asset(gltf_asset *out_asset){
  if(out_asset->copyright != NULL){
    vfree(out_asset->copyright, vstrlen(out_asset->copyright)+1, MEMORY_TAG_STRING);
  }
  if(out_asset->generator != NULL){
    vfree(out_asset->generator, vstrlen(out_asset->generator)+1, MEMORY_TAG_STRING);
  }
  if(out_asset->minVersion != NULL){
    vfree(out_asset->minVersion, vstrlen(out_asset->minVersion)+1, MEMORY_TAG_STRING);
  }
  vfree(out_asset->version, vstrlen(out_asset->version)+1, MEMORY_TAG_STRING);
}

void free_gltf_sampler(gltf_sampler *out_sampler){
  if(out_sampler->name != NULL){
    vfree(out_sampler->name, vstrlen(out_sampler->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_node(gltf_node *out_node){
  if(out_node->children != NULL){
    vfree(out_node->children, sizeof(u64)*out_node->childCount, MEMORY_TAG_GLTF);
  }
  if(out_node->weights != NULL){
    vfree(out_node->weights, sizeof(f64)*out_node->weightCount, MEMORY_TAG_GLTF);
  }
  if(out_node->name != NULL){
    vfree(out_node->name, vstrlen(out_node->name)+1, MEMORY_TAG_STRING);
  }
}

void free_gltf_scene(gltf_scene *out_scene){
  if(out_scene->nodes != NULL){
    vfree(out_scene->nodes, sizeof(u64)*out_scene->nodeCount, MEMORY_TAG_GLTF);
  }
  if(out_scene->name != NULL){
    vfree(out_scene->name, vstrlen(out_scene->name)+1, MEMORY_TAG_GLTF);
  }
}

void free_gltf(gltf_object* out_gltf){
  free_gltf_asset(&out_gltf->asset);
  if(out_gltf->buffers != NULL){
    for(int i = 0; i < out_gltf->bufferCount; i++){
      free_gltf_buffer(&out_gltf->buffers[i]);
    }
    vfree(out_gltf->buffers, out_gltf->bufferCount*sizeof(gltf_buffer), MEMORY_TAG_GLTF);
  }
  
  if(out_gltf->bufferViews != NULL){
    for(int i = 0; i < out_gltf->bufferViewCount; i++){
      free_gltf_buffer_view(&out_gltf->bufferViews[i]);
    }
    vfree(out_gltf->bufferViews, out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_GLTF);
  }

  if(out_gltf->accessors != NULL){
    for(int i = 0; i < out_gltf->accessorCount; i++){
      free_gltf_accessor(&out_gltf->accessors[i]);
    }
    vfree(out_gltf->accessors, out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_GLTF);
  }

  if(out_gltf->images != NULL){
    for(int i = 0; i < out_gltf->imageCount; i++){
      free_gltf_image(&out_gltf->images[i]);
    }
    vfree(out_gltf->images, out_gltf->imageCount*sizeof(gltf_image), MEMORY_TAG_GLTF);
  }

  if(out_gltf->textures != NULL){
    for(int i = 0; i < out_gltf->textureCount; i++){
      free_gltf_texture(&out_gltf->textures[i]);
    }
    vfree(out_gltf->textures, out_gltf->textureCount*sizeof(gltf_texture), MEMORY_TAG_GLTF);
  }

  if(out_gltf->materials != NULL){
    for(int i = 0; i < out_gltf->materialCount; i++){
      free_gltf_material(&out_gltf->materials[i]);
    }
    vfree(out_gltf->materials, out_gltf->materialCount*sizeof(gltf_material), MEMORY_TAG_GLTF);
  }

  if(out_gltf->meshes != NULL){
    for(int i = 0; i < out_gltf->meshCount; i++){
      free_gltf_mesh(&out_gltf->meshes[i]);
    }
    vfree(out_gltf->meshes, sizeof(gltf_mesh)*out_gltf->meshCount, MEMORY_TAG_GLTF);
  }

  if(out_gltf->animations != NULL){
    for(int i = 0; i < out_gltf->animationCount; i++){
      free_gltf_animation(&out_gltf->animations[i]);
    }
    vfree(out_gltf->animations, sizeof(gltf_animation)*out_gltf->animationCount, MEMORY_TAG_GLTF);
  }
  
  if(out_gltf->cameras != NULL){
    for(int i = 0; i < out_gltf->cameraCount; i++){
      free_gltf_camera(&out_gltf->cameras[i]);
    }
    vfree(out_gltf->cameras, sizeof(gltf_camera)*out_gltf->cameraCount, MEMORY_TAG_GLTF);
  }

  if(out_gltf->skins != NULL){
    for(int i = 0; i < out_gltf->skinCount; i++){
      free_gltf_skin(&out_gltf->skins[i]);
    }
    vfree(out_gltf->skins, sizeof(gltf_skin)*out_gltf->skinCount, MEMORY_TAG_GLTF);
  }

  if(out_gltf->samplers != NULL){
    for(int i = 0; i < out_gltf->samplerCount; i++){
      free_gltf_sampler(&out_gltf->samplers[i]);
    }
    vfree(out_gltf->samplers, sizeof(gltf_sampler)*out_gltf->samplerCount, MEMORY_TAG_GLTF);
  }

  if(out_gltf->nodes != NULL){
    for(int i = 0; i < out_gltf->nodeCount; i++){
      free_gltf_node(&out_gltf->nodes[i]);
    }
    vfree(out_gltf->nodes, sizeof(gltf_node)*out_gltf->nodeCount, MEMORY_TAG_GLTF);
  }

  if(out_gltf->scenes != NULL){
    for(int i = 0; i < out_gltf->sceneCount; i++){
      free_gltf_scene(&out_gltf->scenes[i]);
    }
    vfree(out_gltf->scenes, sizeof(gltf_scene)*out_gltf->sceneCount, MEMORY_TAG_GLTF);
  }
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
  }
    
  #define GLTF_I8 5120
#define GLTF_U8 5121
#define GLTF_I16 5122
#define GLTF_U16 5123
#define GLTF_I32 5124
#define GLTF_U32 5125
#define GLTF_FLOAT 5126*/

b8 gltf_accessor_get_element_size(gltf_object* gltf, u64 accIndex, u64 *outSize, gltf_data_type *outType){
  if(accIndex >= gltf->accessorCount){
    VERROR("GLTF accessor index %d is out of bounds, max index is %d", accIndex, gltf->accessorCount-1);
    return FALSE;
  }
  gltf_accessor curAcc = gltf->accessors[accIndex];
  if(vstrcmp(curAcc.type, "SCALAR") == FALSE && curAcc.componentType != GLTF_FLOAT){
    VERROR("Velora doesn't support GLTF files that have vectors or matricies that aren't floats");
    return FALSE;
  }
  gltf_data_type scratchType = 0;
  u64 scratchSize = 0;
  if(vstrcmp(curAcc.type, "MAT2")){
    scratchType = MAT2D;
    scratchSize = sizeof(mat2);
  }else if(vstrcmp(curAcc.type, "MAT3")){
    scratchType = MAT3D;
    scratchSize = sizeof(mat3);
  }else if(vstrcmp(curAcc.type, "MAT4")){
    scratchType = MAT4D;
    scratchSize = sizeof(mat4);
  }else if(vstrcmp(curAcc.type, "VEC2")){
    scratchType = VECTOR2D;
    scratchSize = sizeof(vec2);
  }else if(vstrcmp(curAcc.type, "VEC3")){
    scratchType = VECTOR3D;
    scratchSize = sizeof(vec3);
  }else if(vstrcmp(curAcc.type, "VEC4")){
    scratchType = VECTOR4D;
    scratchSize = sizeof(vec4);
  }else if(vstrcmp(curAcc.type, "SCALAR")){
    switch (curAcc.componentType){
      case GLTF_I8:
        scratchType = I8;
        scratchSize = sizeof(i8);
      break;
      case GLTF_U8:
        scratchType = U8;
        scratchSize = sizeof(u8);
      break;
      case GLTF_I16:
        scratchType = I16;
        scratchSize = sizeof(i16);
      break;
      case GLTF_U16:
        scratchType = U16;
        scratchSize = sizeof(u16);
      break;
      case GLTF_I32:
        scratchType = I32;
        scratchSize = sizeof(i32);
      break;
      case GLTF_U32:
        scratchType = U32;
        scratchSize = sizeof(u32);
      break;
      case GLTF_FLOAT:
        scratchType = FLOAT;
        scratchSize = sizeof(f32);
      break;
      default:
        VERROR("GLTF accessor %d has invalid component type", accIndex);
        return FALSE;
      break;
    }
  }
  if(outSize != NULL){
    (*outSize) = scratchSize;
  }
  if(outType != NULL){
    (*outType) = scratchType;
  }
  return TRUE;
}

b8 gltf_accessor_get_element_count(gltf_object* gltf, u64 accIndex, u64 *outCount){
  if(accIndex >= gltf->accessorCount){
    VERROR("GLTF accessor index %d is out of bounds, max index is %d", accIndex, gltf->accessorCount-1);
    return FALSE;
  }
  (*outCount) = gltf->accessors[accIndex].count;
  return TRUE;
}

b8 gltf_get_stream(gltf_object* gltf, u64 accIndex, gltf_data_stream *outStream){
  if(accIndex >= gltf->accessorCount){
    VERROR("GLTF accessor index %d is out of bounds, max index is %d", accIndex, gltf->accessorCount-1);
    return FALSE;
  }
  gltf_accessor curAcc = gltf->accessors[accIndex];
  if(curAcc.bufferViewIndex >= gltf->bufferViewCount){
    VERROR("GLTF buffer view index %d is out of bounds, max index is %d", curAcc.bufferViewIndex, gltf->bufferViewCount-1);
    return FALSE;
  }
  gltf_buffer_view curView = gltf->bufferViews[curAcc.bufferViewIndex];
  if(curView.bufferIndex >= gltf->bufferCount){
    VERROR("GLTF buffer index %d is out of bounds, max index is %d", curView.bufferIndex, gltf->bufferCount-1);
    return FALSE;
  }
  gltf_data_stream scratchStream = {0};
  gltf_buffer curBuf = gltf->buffers[curView.bufferIndex];
  scratchStream.count = curAcc.count;
  VEL_CHECK(gltf_accessor_get_element_size(gltf, accIndex, &scratchStream.dataSize, &scratchStream.type));
  scratchStream.data = vallocate(scratchStream.dataSize*scratchStream.count, MEMORY_TAG_GLTF);
  u8 *buffer = curBuf.buffer + curView.offset;
  u64 dataStride = scratchStream.dataSize;
  if(curView.stride != 0){
    dataStride = curView.stride;
  }
  for(int i = 0; i < curAcc.count; i++){
    if(curView.offset+(dataStride*i)+scratchStream.dataSize > curBuf.size){
      VERROR("GLTF Accessor went beyond the bounds of the buffer. Buffer size %d, attempted access at %d", curBuf.size, curView.offset+(dataStride*i)+scratchStream.dataSize);
      gltf_free_stream(scratchStream);
      return FALSE;
    }
    vcopy_memory(scratchStream.data+(scratchStream.dataSize*i), buffer+(dataStride*i), scratchStream.dataSize);
  }
  vcopy_memory(outStream, &scratchStream, sizeof(scratchStream));
  return TRUE;
}

b8 gltf_accessor_get_u32_array(gltf_object* gltf, u64 accIndex, u32 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == U8){
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = scratchStream.data[i];
    }
  }else if(scratchStream.type == U16){
    u16 *arr = (u16*)scratchStream.data;
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = arr[i];
    }
  }else if(scratchStream.type == U32){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not able to be put in a u32 array", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_vec2_array(gltf_object* gltf, u64 accIndex, vec2 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == VECTOR2D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 2D Vector type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_vec3_array(gltf_object* gltf, u64 accIndex, vec3 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == VECTOR3D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 3D Vector type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_vec4_array(gltf_object* gltf, u64 accIndex, vec4 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == VECTOR4D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 4D Vector type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_mat4_array(gltf_object* gltf, u64 accIndex, mat4 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == MAT4D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 4x4 matrix type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_u8_array(gltf_object* gltf, u64 accIndex, u8 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == U8){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to an unsigned 8 bit integer type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_i8_array(gltf_object* gltf, u64 accIndex, i8 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == I8){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a signed 8 bit integer type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_u16_array(gltf_object* gltf, u64 accIndex, u16 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == U8){
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = scratchStream.data[i];
    }
  }else if(scratchStream.type == U16){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not able to be put in a u16 array", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_i16_array(gltf_object* gltf, u64 accIndex, i16 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == I8){
    i8 *arr = (i8*)scratchStream.data;
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = arr[i];
    }
  }else if(scratchStream.type == I16){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not able to be put in an i16 array", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_i32_array(gltf_object* gltf, u64 accIndex, i32 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == I8){
    i8 *arr = (i8*)scratchStream.data;
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = arr[i];
    }
  }else if(scratchStream.type == I16){
    i16 *arr = (i16*)scratchStream.data;
    for(int i = 0; i < scratchStream.count; i++){
      outArray[i] = arr[i];
    }
  }else if(scratchStream.type == I32){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not able to be put in an i32 array", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_f32_array(gltf_object* gltf, u64 accIndex, f32 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == FLOAT){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a floating point type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_mat2_array(gltf_object* gltf, u64 accIndex, mat2 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == MAT2D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 2x2 matrix type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

b8 gltf_accessor_get_mat3_array(gltf_object* gltf, u64 accIndex, mat3 *outArray){
  gltf_data_stream scratchStream = {0};
  VEL_CHECK(gltf_get_stream(gltf, accIndex, &scratchStream));
  if(scratchStream.type == MAT3D){
    vcopy_memory(outArray, scratchStream.data, scratchStream.dataSize*scratchStream.count);
  }else{
    VERROR("Accessor %d is not pointing to a 3x3 matrix type", accIndex);
    gltf_free_stream(scratchStream);
    return FALSE;
  }
  gltf_free_stream(scratchStream);
  return TRUE;
}

void gltf_free_stream(gltf_data_stream stream){
  vfree(stream.data, stream.dataSize*stream.count, MEMORY_TAG_GLTF);
}