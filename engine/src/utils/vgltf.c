#include "vgltf.h"
#include "utils/vjson.h"
#include "utils/vfile.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "utils/vstring.h"
//#define __STDC_NO_THREADS__
#ifndef __STDC_NO_THREADS__
#include <threads.h>
typedef struct _image_thread_data{
  char* uri;
  velora_pixels* out_image;
}image_thread_data;
#endif

b8 extract_gltf_buffer(json_value* buffer, gltf_buffer* out_buffer, const char* uriPath){
  if(buffer->type != VELORA_JSON_OBJECT){
    VERROR("Buffer in GLTF File isn't an object");
    return FALSE;
  }
  json_value bufferSize = {0};
  json_value bufferUri = {0};
  VEL_LOAD_JSON_VALUE(buffer->data.object, "byteLength", bufferSize, VELORA_JSON_INTEGER);
  VEL_LOAD_JSON_VALUE(buffer->data.object, "uri", bufferUri, VELORA_JSON_STRING);
  out_buffer->size = bufferSize.data.integer;
  out_buffer->buffer = vallocate(bufferSize.data.integer, MEMORY_TAG_GLTF);
  char * fullUri = vconcat(uriPath, bufferUri.data.string);
  velora_file bufferContents = {0};
  VEL_CHECK_MSG(get_file_contents(fullUri, &bufferContents), "Unable to get contents of buffer file with URI %s", fullUri);
  vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
  vcopy_memory(out_buffer->buffer, bufferContents.contents, out_buffer->size);
  free_velora_file(&bufferContents);
  free_json_value(&bufferUri);
  return TRUE;
}

b8 extract_gltf_buffer_view(json_value* buffer_view, gltf_buffer_view* out_view, gltf_object* obj){
  json_value bufferIndex = {0};
  json_value length = {0};
  VEL_LOAD_JSON_VALUE(buffer_view->data.object, "buffer", bufferIndex, VELORA_JSON_INTEGER);
  VEL_LOAD_JSON_VALUE(buffer_view->data.object, "byteLength", length, VELORA_JSON_INTEGER);
  u64 type = 0;
  VEL_LOAD_OPTIONAL_JSON_INTEGER(buffer_view->data.object, "target", type);
  u64 stride = 0;
  VEL_LOAD_OPTIONAL_JSON_INTEGER(buffer_view->data.object, "byteStride", stride);
  u64 offset = 0;
  VEL_LOAD_OPTIONAL_JSON_INTEGER(buffer_view->data.object, "byteOffset", offset);
  if(bufferIndex.data.integer >= obj->bufferCount){
    VERROR("Buffer view references a buffer that doesn't exist");
    VERROR("Buffer count: %s", obj->bufferCount);
    VERROR("Buffer index: %d", bufferIndex.data.integer);
    return FALSE;
  }
  if(offset > obj->buffers[bufferIndex.data.integer].size || offset+length.data.integer > obj->buffers[bufferIndex.data.integer].size){
    VERROR("Buffer view goes beyond the size of the buffer that is referenced");
    VERROR("Reference buffer size: %d", obj->buffers[bufferIndex.data.integer].size);
    VERROR("Buffer view offset: %d", offset);
    VERROR("Buffer view size: %d", length.data.integer);
    VERROR("End of buffer view: %d", offset+length.data.integer);
    return FALSE;
  }
  out_view->size = length.data.integer;
  out_view->buffer = obj->buffers[bufferIndex.data.integer].buffer+offset;
  out_view->type = type;
  out_view->stride = stride;

  return TRUE;
}

b8 extract_gltf_accessor(json_value* accessor, gltf_accessor *out_acc, gltf_object* obj){
  json_value bViewIndex = {0};
  json_value cType = {0};
  json_value count = {0};
  json_value vMax = {0};
  json_value vMin = {0};
  json_value type = {0};
  VEL_LOAD_JSON_VALUE(accessor->data.object, "bufferView", bViewIndex, VELORA_JSON_INTEGER);
  out_acc->offset = 0;
  VEL_LOAD_OPTIONAL_JSON_INTEGER(accessor->data.object, "byteOffset", out_acc->offset);
  out_acc->normalized = FALSE;
  VEL_LOAD_OPTIONAL_JSON_INTEGER(accessor->data.object, "normalized", out_acc->normalized);
  VEL_LOAD_JSON_VALUE(accessor->data.object, "componentType", cType, VELORA_JSON_INTEGER);
  VEL_LOAD_JSON_VALUE(accessor->data.object, "count", count, VELORA_JSON_INTEGER);
  VEL_LOAD_JSON_VALUE(accessor->data.object, "type", type, VELORA_JSON_STRING);
  if(bViewIndex.data.integer >= obj->bufferViewCount){
    VERROR("Accessor has buffer view index that doesn't exist");
    VERROR("Requested buffer view index: %d", bViewIndex.data.integer);
    VERROR("Number of buffer views: %d", obj->bufferViewCount);
    return FALSE;
  }
  out_acc->bufferView = &obj->bufferViews[bViewIndex.data.integer];
  out_acc->componentType = cType.data.integer;
  out_acc->count = count.data.integer;
  out_acc->type = vallocate(type.dataSize, MEMORY_TAG_GLTF);
  vcopy_memory(out_acc->type, type.data.string, type.dataSize);
  out_acc->max_count = 0;
  out_acc->max = NULL;
  if(get_json_value(accessor->data.object, "max", &vMax) == TRUE && vMax.type == VELORA_JSON_ARRAY){
    u64 maxCount = vMax.dataSize/sizeof(json_value);
    out_acc->max_count = maxCount;
    out_acc->max = vallocate(sizeof(gltf_value)*maxCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < maxCount; i++){
      if(out_acc->componentType == GLTF_FLOAT){
        out_acc->max[i].dFloat = vMax.data.array[i].data.dFloat; 
      }else if(out_acc->componentType % 2 == 0){
        out_acc->max[i].signedInteger = vMax.data.array[i].data.integer;
      }else if(out_acc->componentType % 2 == 1){
        out_acc->max[i].unsignedInteger = vMax.data.array[i].data.integer;
      }
    }
    free_json_value(&vMax);
  }
  out_acc->min_count = 0;
  out_acc->min = NULL;
  if(get_json_value(accessor->data.object, "min", &vMin) == TRUE && vMin.type == VELORA_JSON_ARRAY){
    u64 minCount = vMin.dataSize/sizeof(json_value);
    out_acc->min_count = minCount;
    out_acc->min = vallocate(sizeof(gltf_value)*minCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < minCount; i++){
      if(out_acc->componentType == GLTF_FLOAT){
        out_acc->min[i].dFloat = vMin.data.array[i].data.dFloat; 
      }else if(out_acc->componentType % 2 == 0){
        out_acc->min[i].signedInteger = vMin.data.array[i].data.integer;
      }else if(out_acc->componentType % 2 == 1){
        out_acc->min[i].unsignedInteger = vMin.data.array[i].data.integer;
      }
    }
    free_json_value(&vMin);
  }
  free_json_value(&type);
  free_json_value(&bViewIndex);
  free_json_value(&cType);
  free_json_value(&count);
  return TRUE;
}

#ifndef __STDC_NO_THREADS__
int import_image_thread(void* data){
  image_thread_data *imageData = (image_thread_data*)data;
  VEL_CHECK_MSG(import_pixels(imageData->uri, imageData->out_image), "Unable to import image %s", imageData->uri);
  vfree(imageData->uri, vstrlen(imageData->uri)+1, MEMORY_TAG_STRING);
  vfree(data, sizeof(image_thread_data), MEMORY_TAG_GLTF);
  return TRUE;
}
#endif

b8 import_gltf(const char *uri, gltf_object *out_gltf){
  velora_file gltfFile = {0};
  if(get_file_contents(uri, &gltfFile) == FALSE){
    VERROR("Unable to read GLTF file %s", uri);
    return FALSE;
  }
  char* uriPath = get_file_path(uri);

  json_value images = {0};
  out_gltf->imageCount = 0;
  out_gltf->images = NULL;
  #ifndef __STDC_NO_THREADS__
  thrd_t *threadIds = NULL;
  #endif
  if(get_json_value(gltfFile.contents, "images", &images) == TRUE && images.type == VELORA_JSON_ARRAY){
    out_gltf->imageCount = images.dataSize/sizeof(json_value);
    #ifndef __STDC_NO_THREADS__
    threadIds = vallocate(sizeof(thrd_t)*out_gltf->imageCount, MEMORY_TAG_GLTF);
    #endif
    out_gltf->images = vallocate(sizeof(velora_pixels)*out_gltf->imageCount, MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->imageCount; i++){
      json_value posImage = images.data.array[i];
      if(posImage.type == VELORA_JSON_OBJECT){
        json_value imageUriJson = {0};
        if(get_json_value(posImage.data.object, "uri", &imageUriJson) == TRUE && imageUriJson.type == VELORA_JSON_STRING){   
          char* fullUri = vconcat(uriPath, imageUriJson.data.string); 
          #ifdef __STDC_NO_THREADS__
          VEL_CHECK_MSG(import_pixels(fullUri, &out_gltf->images[i]), "Unable to import image %s", fullUri);
          vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
          #else
          image_thread_data *dat = vallocate(sizeof(image_thread_data), MEMORY_TAG_GLTF);
          dat->uri = fullUri;
          dat->out_image = &out_gltf->images[i];
          thrd_create(threadIds+i, import_image_thread, dat);
          #endif
          free_json_value(&imageUriJson);
        }
      }
    }
    free_json_value(&images);
  }
  json_value buffers = {0};
  VEL_CHECK_MSG(get_json_value(gltfFile.contents, "buffers", &buffers), "GLTF File %s doesn't have a buffers variable", uri);
  if(buffers.type != VELORA_JSON_ARRAY){
    VERROR("GLTF File %s has the buffers variable as something other than an array", uri);
    return FALSE;
  }

  u64 buffersLength = buffers.dataSize/sizeof(json_value);
  out_gltf->bufferCount = buffersLength;
  out_gltf->buffers = vallocate(sizeof(gltf_buffer)*out_gltf->bufferCount, MEMORY_TAG_GLTF);
  for(int i = 0; i < buffersLength; i++){
    if(uriPath == NULL){
      VEL_CHECK_MSG(extract_gltf_buffer(&buffers.data.array[i], &out_gltf->buffers[i], "") == FALSE, "GLTF File %s has a malformed buffer object at index %d", uri, i);
    }else{
      VEL_CHECK_MSG(extract_gltf_buffer(&buffers.data.array[i], &out_gltf->buffers[i], uriPath), "GLTF File %s has a malformed buffer object at index %d", uri, i);
    }
  }
  free_json_value(&buffers);
  
  if(uriPath != NULL){
    vfree(uriPath, vstrlen(uriPath)+1, MEMORY_TAG_STRING);
  }

  json_value bufferViews = {0};
  VEL_CHECK_MSG(get_json_value(gltfFile.contents, "bufferViews", &bufferViews), "No bufferViews variable in GLTF file %s", uri);
  if(bufferViews.type != VELORA_JSON_ARRAY){
    VERROR("bufferViews in GLTF file %s is not an array", uri);
    return FALSE;
  }
  out_gltf->bufferViewCount = bufferViews.dataSize/sizeof(json_value);
  out_gltf->bufferViews = vallocate(out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->bufferViewCount; i++){
    VEL_CHECK_MSG(extract_gltf_buffer_view(&bufferViews.data.array[i], &out_gltf->bufferViews[i], out_gltf), "GLTF File %s has malformed buffer view object at index %d", uri, i);
  }
  free_json_value(&bufferViews);
  
  json_value accessors = {0};
  VEL_CHECK_MSG(get_json_value(gltfFile.contents, "accessors", &accessors), "No accessors variable in GLTF file %s", uri);
  if(accessors.type != VELORA_JSON_ARRAY){
    VERROR("accessors in GLTF file %s is not an array", uri);
    return FALSE;
  }
  out_gltf->accessorCount = accessors.dataSize/sizeof(json_value);
  out_gltf->accessors = vallocate(out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->accessorCount; i++){
    VEL_CHECK_MSG(extract_gltf_accessor(&accessors.data.array[i], &out_gltf->accessors[i], out_gltf), "GLTF File %s has malformed accessor object at index %d", uri, i);
  }
  free_json_value(&accessors);

  json_value textures = {0};
  if(get_json_value(gltfFile.contents, "textures", &textures) == TRUE){
    if(textures.type != VELORA_JSON_ARRAY){
      VERROR("textures in GLTF File %s is not an array", uri);
      free_json_value(&textures);
    }
    out_gltf->textureCount = textures.dataSize/sizeof(json_value);
    out_gltf->textures = vallocate(out_gltf->textureCount*sizeof(gltf_texture), MEMORY_TAG_GLTF);
    for(int i = 0; i < out_gltf->textureCount; i++){
      json_value *curTex = &textures.data.array[i];
      if(curTex->type != VELORA_JSON_OBJECT){
        VERROR("texture in GLTF File %s is not an array of objects", uri);
        break;
      }
      gltf_texture *outTexture = &out_gltf->textures[i];
      outTexture->sampler = -1;
      outTexture->source = -1;
      json_value sourceSampler = {0};
      if(get_json_value(curTex->data.object, "source", &sourceSampler) == TRUE && sourceSampler.type == VELORA_JSON_INTEGER){
        outTexture->source = sourceSampler.data.integer;
      }
      if(get_json_value(curTex->data.object, "sampler", &sourceSampler) == TRUE && sourceSampler.type == VELORA_JSON_INTEGER){
        outTexture->sampler = sourceSampler.data.integer;
      }
      json_value name = {0};
      if(get_json_value(curTex->data.object, "name", &name) == TRUE && name.type == VELORA_JSON_STRING){
        outTexture->name = vallocate(name.dataSize, MEMORY_TAG_GLTF);
        vcopy_memory(outTexture->name, name.data.string, name.dataSize);
        free_json_value(&name);
      }
    }
  }

  json_value materials = {0};
  if(get_json_value(gltfFile.contents, "materials", &materials) == TRUE){
    if(materials.type != VELORA_JSON_ARRAY){
      VERROR("materials in GLTF File %s is not an array", uri);
      free_json_value(&materials);
    }
  }

  free_velora_file(&gltfFile);
  b8 retVal = TRUE;
  #ifndef __STDC_NO_THREADS__
  if(threadIds == NULL){
    return TRUE;
  }
  for(int i = 0; i < out_gltf->imageCount; i++){
    int result;
    thrd_join(threadIds[i], &result);
    if(result == FALSE){
      VERROR("Unable to load image at index %d in gltf file %s", i, uri);
      retVal = FALSE;
    }
  }
  #endif
  return retVal;
}

void free_gltf_buffer(gltf_buffer* buf){
  vfree(buf->buffer, buf->size, MEMORY_TAG_GLTF);
}

void free_gltf(gltf_object* out_gltf){
  for(int i = 0; i < out_gltf->bufferCount; i++){
    free_gltf_buffer(&out_gltf->buffers[i]);
  }
  vfree(out_gltf->buffers, out_gltf->bufferCount*sizeof(gltf_buffer), MEMORY_TAG_GLTF);
  vfree(out_gltf->bufferViews, out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_GLTF);
  for(int i = 0; i < out_gltf->accessorCount; i++){
    gltf_accessor *curAccessor = &out_gltf->accessors[i];
    vfree(curAccessor->type, vstrlen(curAccessor->type)+1, MEMORY_TAG_GLTF);
    if(curAccessor->max_count != 0){
      vfree(curAccessor->max, curAccessor->max_count*sizeof(gltf_value), MEMORY_TAG_GLTF);
    }
    if(curAccessor->min_count != 0){
      vfree(curAccessor->min, curAccessor->min_count*sizeof(gltf_value), MEMORY_TAG_GLTF);
    }
  }
  vfree(out_gltf->accessors, out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_GLTF);
  vfree(out_gltf->images, out_gltf->imageCount*sizeof(velora_pixels), MEMORY_TAG_GLTF);
}
