#include "vgltf.h"
#include "utils/vjson.h"
#include "utils/vfile.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "utils/vstring.h"
//#define __STDC_NO_THREADS__
#ifndef __STDC_NO_THREADS__
#include <threads.h>
#endif

static const char * EXTENSIONS_SUPPORTED[] = {
  "None",
};
static const u64 EXTENSIONS_SUPPORTED_COUNT = 1;

b8 extract_gltf_buffer(json_value* buffer, gltf_buffer* out_buffer, const char* uriPath){
  if(buffer->type != VELORA_JSON_OBJECT){
    VERROR("Buffer in GLTF File isn't an object");
    return FALSE;
  }
  char *bufferUri = NULL;
  VEL_CHECK(load_json_string(buffer, "uri", &bufferUri));
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

b8 extract_gltf_buffer_view(json_value* buffer_view, gltf_buffer_view* out_view){
  // These two are required, return FALSE if they're not here
  VEL_CHECK(load_json_unsigned_integer(buffer_view, "buffer", &out_view->bufferIndex));
  VEL_CHECK(load_json_unsigned_integer(buffer_view, "byteLength", &out_view->size));
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
  VEL_CHECK(load_json_object(sparseAccessor, "indices", &indices));
  VEL_CHECK(load_json_object(sparseAccessor, "values", &values));
  VEL_CHECK(load_json_unsigned_integer(sparseAccessor, "count", &out_acc->count));
  
  //Load the indicies
  VEL_CHECK(load_json_unsigned_integer(&indices, "bufferView", &out_acc->indices.bufferViewIndex));
  VEL_CHECK(load_json_unsigned_integer(&indices, "componentType", &out_acc->indices.cType));
  out_acc->indices.byteOffset = 0;
  load_json_unsigned_integer(&indices, "byteOffset", &out_acc->indices.byteOffset);

  //Load the values
  VEL_CHECK(load_json_unsigned_integer(&values, "bufferView", &out_acc->values.bufferViewIndex));
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
    }
  }else if(cType%2 == 0){ // Signed
    i64 *valueArray = NULL;
    if(load_json_signed_integer_array(accessor, variable, &valueArray, count) == TRUE){
      (*outArr) = vallocate(sizeof(gltf_value)*(*count), MEMORY_TAG_GLTF);
      for(int i = 0; i < (*count); i++){
        (*outArr)[i].signedInteger = valueArray[i];
      }
      vfree(valueArray, sizeof(i64)*(*count), MEMORY_TAG_JSON);
    }
  }else if(cType%2 == 1){ // Unsigned
    u64 *valueArray = NULL;
    if(load_json_unsigned_integer_array(accessor, variable, &valueArray, count) == TRUE){
      (*outArr) = vallocate(sizeof(gltf_value)*(*count), MEMORY_TAG_GLTF);
      for(int i = 0; i < (*count); i++){
        (*outArr)[i].unsignedInteger = valueArray[i];
      }
      vfree(valueArray, sizeof(u64)*(*count), MEMORY_TAG_JSON);
    }
  }
  return TRUE;
}

b8 extract_gltf_accessor(json_value* accessor, gltf_accessor *out_acc){
  json_value sparseObj = {0};
  b8 sparseAccessorExists = load_json_object(accessor, "sparse", &sparseObj);
  out_acc->bufferViewIndex = -1;
  if(
    load_json_signed_integer(accessor, "bufferView", &out_acc->bufferViewIndex) == FALSE && 
    sparseAccessorExists == FALSE
  ){
    VERROR("No bufferView or sparse variable in accessor");
    return FALSE;
  }
  out_acc->offset = 0;
  load_json_unsigned_integer(accessor, "byteOffset", &out_acc->offset);
  VEL_CHECK(load_json_unsigned_integer(accessor, "componentType", &out_acc->componentType));
  u64 normalizedBool = FALSE;
  load_json_unsigned_integer(accessor, "normalized", &normalizedBool);
  out_acc->normalized = normalizedBool;
  VEL_CHECK(load_json_unsigned_integer(accessor, "count", &out_acc->count));
  VEL_CHECK(load_json_string(accessor, "type", &out_acc->type));
  VEL_CHECK(extract_gltf_value_array(accessor, "max", &out_acc->max, &out_acc->max_count, out_acc->componentType));
  VEL_CHECK(extract_gltf_value_array(accessor, "min", &out_acc->min, &out_acc->min_count, out_acc->componentType));
  out_acc->name = NULL;
  load_json_string(accessor, "name", &out_acc->name);
  if(sparseAccessorExists == TRUE){
    VEL_CHECK(extract_gltf_sparse_accessor(&sparseObj, &out_acc->sparse));
  }else{
    out_acc->sparse.count = 0;
  }
  return TRUE;
}

b8 extract_gltf_image(json_value* image, gltf_image *out_image, const char* uriPath){
  char *uri = NULL;
  if(load_json_string(image, "uri", &uri) == TRUE){
    char *fullUri = vconcat(uriPath, uri);
    VEL_CHECK(import_pixels(fullUri, &out_image->uriData));
    vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
    vfree(uri, vstrlen(uri)+1, MEMORY_TAG_STRING);
    out_image->mimeType = NULL;
    out_image->bufferViewIndex = U64_MAX;
  }else{
    VEL_CHECK(load_json_string(image, "mimeType", &out_image->mimeType));
    VEL_CHECK(load_json_unsigned_integer(image, "bufferView", &out_image->bufferViewIndex));
    out_image->uriData.size = 0;
  }
  out_image->name = NULL;
  load_json_string(image, "name", &out_image->name);
  return TRUE;
}

b8 extract_gltf_texture(json_value* texture, gltf_texture *out_texture){
  return TRUE;
}

b8 extract_gltf_material(json_value* material, gltf_material *out_material){
  return TRUE;
}

#ifndef __STDC_NO_THREADS__
//json_value* buffer, gltf_buffer* out_buffer, const char* uriPath
typedef struct _buffer_thread_data{
  json_value *bufferObj;
  gltf_buffer *outBuffer;
  char *uriPath;
}buffer_thread_data;
int import_buffer_thread(void* data){
  buffer_thread_data *bufData = (buffer_thread_data*)data;
  VEL_CHECK(extract_gltf_buffer(bufData->bufferObj, bufData->outBuffer, bufData->uriPath));
  vfree(data, sizeof(buffer_thread_data), MEMORY_TAG_GLTF);
  return TRUE;
}

//json_value* buffer_view, gltf_buffer_view* out_view
typedef struct _buffer_view_thread_data{
  json_value *bufferViewObj;
  gltf_buffer_view *outView;
}buffer_view_thread_data;
int import_buffer_view_thread(void* data){
  buffer_view_thread_data *bufData = (buffer_view_thread_data*)data;
  VEL_CHECK(extract_gltf_buffer_view(bufData->bufferViewObj, bufData->outView));
  vfree(data, sizeof(buffer_view_thread_data), MEMORY_TAG_GLTF);
  return TRUE;
}

//json_value* accessor, gltf_accessor *out_acc
typedef struct _accessor_thread_data{
  json_value *accessor;
  gltf_accessor *outAcc;
}accessor_thread_data;
int import_accessor_thread(void* data){
  accessor_thread_data *accData = (accessor_thread_data*)data;
  VEL_CHECK(extract_gltf_accessor(accData->accessor, accData->outAcc));
  vfree(data, sizeof(accessor_thread_data), MEMORY_TAG_GLTF);
  return TRUE;
}

//json_value* image, gltf_image *out_image, const char* uriPath
typedef struct _image_thread_data{
  json_value *image;
  gltf_image *outImage;
  char *uriPath;
}image_thread_data;
int import_image_thread(void* data){
  image_thread_data* imageData = (image_thread_data*)data;
  VEL_CHECK(extract_gltf_image(imageData->image, imageData->outImage, imageData->uriPath));
  vfree(data, sizeof(image_thread_data), MEMORY_TAG_GLTF);
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

b8 import_gltf(const char *uri, gltf_object *out_gltf){
  #ifdef __STDC_NO_THREADS__
  VFATAL("Compiler doesn't support C11 threads, this is needed to import GLTF Files");
  return FALSE;
  #endif
  velora_file gltfFile = {0};
  if(get_file_contents(uri, &gltfFile) == FALSE){
    VERROR("Unable to read GLTF file %s", uri);
    return FALSE;
  }

  thrd_t threadIds[512];
  u64 threadCount = 0;
  char* uriPath = get_file_path(uri);

  json_value gltfJson = {0};
  gltfJson.data.object = gltfFile.contents;
  gltfJson.dataSize = gltfFile.size;
  gltfJson.type = VELORA_JSON_OBJECT;

  char **extensionsRequired = NULL;
  u64 extentionCount = 0;
  if(load_json_string_array(&gltfJson, "extensionsRequired", &extensionsRequired, &extentionCount) == TRUE){
    if(check_extension_validity(extensionsRequired, extentionCount) == FALSE){
      VERROR("GLTF File %s uses unsupported extension(s), unable to be imported", uri);
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

  json_value *buffers = NULL;
  if(load_json_object_array(&gltfJson, "buffers", &buffers, &out_gltf->bufferCount) == TRUE){
    out_gltf->buffers = vallocate(sizeof(gltf_buffer)*out_gltf->bufferCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->bufferCount; i++){
      buffer_thread_data *bufferData = vallocate(sizeof(buffer_thread_data), MEMORY_TAG_GLTF);
      bufferData->bufferObj = &buffers[i];
      bufferData->outBuffer = &out_gltf->buffers[i];
      bufferData->uriPath = uriPath;
      thrd_create(threadIds+threadCount, import_buffer_thread, bufferData);
      threadCount++;
    }
  }

  json_value *bufferViews = NULL;
  if(load_json_object_array(&gltfJson, "bufferViews", &bufferViews, &out_gltf->bufferViewCount) == TRUE){
    out_gltf->bufferViews = vallocate(sizeof(gltf_buffer_view)*out_gltf->bufferViewCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->bufferViewCount; i++){
      buffer_view_thread_data *bufferViewData = vallocate(sizeof(buffer_view_thread_data), MEMORY_TAG_GLTF);
      bufferViewData->bufferViewObj = &bufferViews[i];
      bufferViewData->outView = &out_gltf->bufferViews[i];
      thrd_create(threadIds+threadCount, import_buffer_view_thread, bufferViewData);
      threadCount++;
    }
  }

  json_value *accessors = NULL;
  if(load_json_object_array(&gltfJson, "accessors", &accessors, &out_gltf->accessorCount) == TRUE){
    out_gltf->accessors = vallocate(sizeof(gltf_accessor)*out_gltf->accessorCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->accessorCount; i++){
      accessor_thread_data *accessorData = vallocate(sizeof(accessor_thread_data), MEMORY_TAG_GLTF);
      accessorData->accessor = &accessors[i];
      accessorData->outAcc = &out_gltf->accessors[i];
      thrd_create(threadIds+threadCount, import_accessor_thread, accessorData);
      threadCount++;
    }
  }

  json_value *images = NULL;
  if(load_json_object_array(&gltfJson, "images", &images, &out_gltf->imageCount) == TRUE){
    out_gltf->images = vallocate(sizeof(gltf_image)*out_gltf->imageCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->imageCount; i++){
      image_thread_data *imageData = vallocate(sizeof(image_thread_data), MEMORY_TAG_GLTF);
      imageData->image = &images[i];
      imageData->outImage = &out_gltf->images[i];
      imageData->uriPath = uriPath;
      thrd_create(threadIds+threadCount, import_image_thread, imageData);
      threadCount++;
    }
  }

  b8 retVal = TRUE;
  for(int i = 0; i < threadCount; i++){
    int result;
    thrd_join(threadIds[i], &result);
    if(result == FALSE){
      VERROR("Unable to load gltf file %s", uri);
      retVal = FALSE;
    }
  }
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

void free_gltf(gltf_object* out_gltf){
  for(int i = 0; i < out_gltf->bufferCount; i++){
    free_gltf_buffer(&out_gltf->buffers[i]);
  }
  vfree(out_gltf->buffers, out_gltf->bufferCount*sizeof(gltf_buffer), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->bufferViewCount; i++){
    free_gltf_buffer_view(&out_gltf->bufferViews[i]);
  }
  vfree(out_gltf->bufferViews, out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->accessorCount; i++){
    free_gltf_accessor(&out_gltf->accessors[i]);
  }
  vfree(out_gltf->accessors, out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->imageCount; i++){
    free_gltf_image(&out_gltf->images[i]);
  }
  vfree(out_gltf->images, out_gltf->imageCount*sizeof(velora_pixels), MEMORY_TAG_GLTF);
}
