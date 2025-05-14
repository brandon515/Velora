#include "vimport.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "utils/vstring.h"
#include "utils/vfile.h"
#include <stdlib.h>
#include "core/stb_image.h"

#define VEL_LOAD_JSON_VALUE(json_object, name, value, expectedValue)                               \
  VEL_CHECK_MSG(get_json_value(json_object, name, &value), "No %s variable", name); \
  if(value.type != expectedValue){                                                                 \
    free_json_value(&value);                                                                       \
    VERROR("variable %s was not an %s", name, #expectedValue);                                     \
    return FALSE;                                                                                  \
  }    

#define VEL_LOAD_OPTIONAL_JSON_INTEGER(json_object, name, variable) \
  {json_value dummy = {0};                                          \
  if(get_json_value(json_object, name, &dummy) == TRUE){            \
    if(dummy.type == VELORA_JSON_INTEGER){                          \
      variable = dummy.data.integer;                                \
    }                                                               \
    free_json_value(&dummy);                                        \
  }}

b8 import_pixels(const char *uri, velora_pixels *out_pixels){
  int height, width, chans;
  out_pixels->pixels = stbi_load(uri, &width, &height, &chans, STBI_rgb_alpha);
  if(!out_pixels->pixels){
    return FALSE;
  }
  out_pixels->width = width;
  out_pixels->height = height;
  // the reason we include 4 here is becuase STBI_rgb_alpha forces the pixels to be 32 bit
  out_pixels->size = out_pixels->width*out_pixels->height*4;
  return TRUE;
}

void free_pixels(velora_pixels *pixels){
  stbi_image_free(pixels->pixels);
}

b8 is_number(char character){
  return (character >= 48) && (character <= 57);
}

b8 extract_json_object(u8* data, json_value *out_object){
  u64 jsonDataSize = 1;
  u64 curLevel = 1, workingLevel = 1;
  while(workingLevel >= curLevel){
    if(data[jsonDataSize] == '}' || data[jsonDataSize] == ']'){
      workingLevel--;
    }else if(data[jsonDataSize] == '{' || data[jsonDataSize] == '['){
      workingLevel++;
    }
    jsonDataSize++;
  }
  out_object->dataSize = jsonDataSize+1;
  out_object->data.object = vallocate(out_object->dataSize, MEMORY_TAG_JSON);
  vcopy_memory(out_object->data.object, data, jsonDataSize);
  out_object->data.object[jsonDataSize] = 0;
  out_object->type = VELORA_JSON_OBJECT;
  return TRUE;
}

b8 extract_json_number(u8 *data, json_value *out_object){
  i8 negativeMul = 1;
  if((*data) == '-'){
    negativeMul = -1;
    data++;
  }
  u64 numberLength = 0;
  while(data[numberLength] != ',' && data[numberLength] >= ' ' && data[numberLength] != '}' && data[numberLength] != ']'){
    numberLength++;
  }
  out_object->type = VELORA_JSON_INTEGER;
  for(int i = 0; i < numberLength; i++){
    if(data[i] == '.'){
      out_object->type = VELORA_JSON_DOUBLE;
    }
  }
  if(out_object->type == VELORA_JSON_INTEGER){ // The value is an integer
    u64 place = 1;
    for(int i = numberLength-1; i >= 0; i--){
      out_object->data.integer += (data[i]-'0')*place;
      place *= 10;
    }
    out_object->data.integer *= negativeMul;
  }else if(out_object->type == VELORA_JSON_DOUBLE){ // The value is a float
    char floatStr[numberLength+1];
    vcopy_memory(floatStr, data, numberLength);
    floatStr[numberLength] = 0;
    out_object->data.dFloat = atof(floatStr);
    out_object->data.dFloat *= negativeMul;
  }
  return TRUE;
}

b8 extract_json_string(u8 *data, json_value *out_object){
  out_object->type = VELORA_JSON_STRING;
  data++;
  u64 valueSize = 0;
  while(data[valueSize] != '"'){
    valueSize++;
  }
  out_object->dataSize = valueSize+1;
  out_object->data.string = vallocate(out_object->dataSize, MEMORY_TAG_JSON);
  vcopy_memory(out_object->data.string, data, valueSize);
  out_object->data.string[valueSize] = 0;
  return TRUE;
}

b8 extract_json_value(u8* data, json_value *out_object){
  char idenChar = (*data);
  if(idenChar == '{'){ // { means it's an object
    return extract_json_object(data, out_object);
  }else if(is_number(idenChar) || idenChar == '-'){// This is a number of some sort, get the ful number and look for a . to see if it's a float
    return extract_json_number(data, out_object);
  }else if(idenChar == '"'){
    return extract_json_string(data, out_object);
  }else if(idenChar == 't'){
    out_object->type = VELORA_JSON_INTEGER;
    out_object->data.integer = TRUE;
    return TRUE;
  }else if(idenChar == 'f'){
    out_object->type = VELORA_JSON_INTEGER;
    out_object->data.integer = FALSE;
    return TRUE;
  }else{
    return FALSE;
  }
}

b8 extract_json_array(u8* data, json_value *out_object){
  u64 jsonDataSize = 1;
  u64 numOfItems = 1;
  u64 workingLevel = 1;
  while(workingLevel >= 1){
    if(data[jsonDataSize] == '}' || data[jsonDataSize] == ']'){
      workingLevel--;
    }else if(data[jsonDataSize] == '{' || data[jsonDataSize] == '['){
      workingLevel++;
    }else if(data[jsonDataSize] == ',' && workingLevel == 1){
      numOfItems++;
    }
    jsonDataSize++;
  }
  out_object->type = VELORA_JSON_ARRAY;
  out_object->dataSize = sizeof(json_value)*numOfItems;
  out_object->data.array = vallocate(out_object->dataSize, MEMORY_TAG_JSON);
  u64 arrayIt = 0;
  jsonDataSize = 1;
  workingLevel = 1;
  data++;
  while((*data) <= ' '){ //clearing out whitespace
    data++;
  }
  VEL_CHECK(extract_json_value(data, &out_object->data.array[arrayIt]));
  arrayIt++;
  while(workingLevel >= 1){
    if((*data) == '}' || (*data) == ']'){
      workingLevel--;
    }else if((*data) == '{' || (*data) == '['){
      workingLevel++;
    }else if(((*data) == ',') && workingLevel == 1){
      data++;
      while((*data) <= ' '){ //clearing out whitespace
        data++;
      }
      if((*data) == '{'){
        workingLevel++;
      }
      VEL_CHECK(extract_json_value(data, &out_object->data.array[arrayIt]));
      arrayIt++;
    }
    data++;
  }
  return TRUE;
}

b8 get_json_value(u8* data, const char *name, json_value *out_object){  // change this to find the json object and return it out a pointer in the function parameters
  u8 *local_array = data;
  if((*local_array) != '{'){
    VERROR("Attempted to parse JSON data without leading {, may be an array or value");
    return FALSE;
  }
  local_array++;
  u64 level = 1;
  while(level > 0){
    u8 character = (*local_array);
    if(character == '{' || character == '['){
      level++; 
    }else if(character == '}' || character == ']'){
      level--;
    }else if(character == '"' && level == 1){
      // See if this is the value we're trying to pull
      u64 stringSize = 0;
      local_array++;
      while(local_array[stringSize] != '"'){
        stringSize++;
      }
      char jsonName[stringSize];
      vcopy_memory(jsonName, local_array, stringSize);
      local_array = local_array+stringSize+1;
      jsonName[stringSize] = 0; //Gotta be sure to zero terminate the string
      if(vstrcmp(jsonName, name) == FALSE){
        continue;
      }
      //This is indeed the value we need
      out_object->data.integer = 0;
      //Get past the space and : 
      while((*local_array) == ' ' || (*local_array) == ':'){
        local_array++;
      }
      char idenChar = (*local_array);
      if(idenChar == '['){
        return extract_json_array(local_array, out_object);
      }else{
        return extract_json_value(local_array, out_object);
      }
    }
    local_array++;
  }
  return FALSE;
}

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
  out_buffer->buffer = vallocate(bufferSize.data.integer, MEMORY_TAG_RENDERER);
  char * fullUri = vconcat(uriPath, bufferUri.data.string);
  velora_file bufferContents = {0};
  VEL_CHECK_MSG(get_file_contents(fullUri, &bufferContents), "Unable to get contents of buffer file with URI %s", fullUri);
  vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
  vcopy_memory(out_buffer->buffer, bufferContents.contents, out_buffer->size);
  free_velora_file(&bufferContents);
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
  out_acc->max_count = 0;
  out_acc->max = NULL;
  if(get_json_value(accessor->data.object, "max", &vMax) == TRUE){
    u64 maxCount = vMax.dataSize/sizeof(json_value);
    out_acc->max_count = maxCount;
    out_acc->max = vallocate(sizeof(gltf_value)*maxCount, MEMORY_TAG_RENDERER);
    for(int i = 0; i < maxCount; i++){
      if(out_acc->componentType == GLTF_FLOAT){
        out_acc->max[i].dFloat = vMax.data.array[i].data.dFloat; 
      }else if(out_acc->componentType == GLTF_U16){
        out_acc->max[i].integer = vMax.data.array[i].data.integer;
      }
    }
    free_json_value(&vMax);
  }
  out_acc->min_count = 0;
  out_acc->min = NULL;
  if(get_json_value(accessor->data.object, "min", &vMin) == TRUE){
    u64 minCount = vMin.dataSize/sizeof(json_value);
    out_acc->min_count = minCount;
    out_acc->min = vallocate(sizeof(gltf_value)*minCount, MEMORY_TAG_RENDERER);
    for(int i = 0; i < minCount; i++){
      if(out_acc->componentType == GLTF_FLOAT){
        out_acc->min[i].dFloat = vMin.data.array[i].data.dFloat; 
      }else if(out_acc->componentType == GLTF_U16){
        out_acc->min[i].integer = vMin.data.array[i].data.integer;
      }
    }
    free_json_value(&vMin);
  }
  out_acc->bufferView = &obj->bufferViews[bViewIndex.data.integer];
  out_acc->componentType = cType.data.integer;
  out_acc->count = count.data.integer;
  out_acc->type = vallocate(type.dataSize, MEMORY_TAG_RENDERER);
  vcopy_memory(out_acc->type, type.data.string, type.dataSize);
  free_json_value(&type);
  return TRUE;
}

b8 import_gltf(const char *uri, gltf_object *out_gltf){
  velora_file gltfFile = {0};
  if(get_file_contents(uri, &gltfFile) == FALSE){
    VERROR("Unable to read GLTF file %s", uri);
    return FALSE;
  }
  char* uriPath = get_file_path(uri);
  
  json_value buffers = {0};
  VEL_CHECK_MSG(get_json_value(gltfFile.contents, "buffers", &buffers), "GLTF File %s doesn't have a buffers variable", uri);
  if(buffers.type != VELORA_JSON_ARRAY){
    VERROR("GLTF File %s has the buffers variable as something other than an array", uri);
    return FALSE;
  }

  u64 buffersLength = buffers.dataSize/sizeof(json_value);
  out_gltf->bufferCount = buffersLength;
  out_gltf->buffers = vallocate(sizeof(gltf_buffer)*out_gltf->bufferCount, MEMORY_TAG_RENDERER);
  for(int i = 0; i < buffersLength; i++){
    if(uriPath == NULL){
      VEL_CHECK_MSG(extract_gltf_buffer(&buffers.data.array[i], &out_gltf->buffers[i], "") == FALSE, "GLTF File %s has a malformed buffer object at index %d", uri, i);
    }else{
      VEL_CHECK_MSG(extract_gltf_buffer(&buffers.data.array[i], &out_gltf->buffers[i], uriPath), "GLTF File %s has a malformed buffer object at index %d", uri, i);
    }
  }
  free_json_value(&buffers);

  json_value images = {0};
  out_gltf->imageCount = 0;
  out_gltf->images = NULL;
  if(get_json_value(gltfFile.contents, "images", &images) == TRUE){
    if(images.type == VELORA_JSON_ARRAY){
      out_gltf->imageCount = images.dataSize/sizeof(json_value);
      out_gltf->images = vallocate(sizeof(velora_pixels)*out_gltf->imageCount, MEMORY_TAG_RENDERER);
      for(int i = 0; i < out_gltf->imageCount; i++){
        json_value posImage = images.data.array[i];
        if(posImage.type == VELORA_JSON_OBJECT){
          json_value imageUriJson = {0};
          if(get_json_value(posImage.data.object, "uri", &imageUriJson) == TRUE && imageUriJson.type == VELORA_JSON_STRING){
            char* fullUri = vconcat(uriPath, imageUriJson.data.string);
            VEL_CHECK_MSG(import_pixels(fullUri, &out_gltf->images[i]), "Unable to import image %s", fullUri);
            free_json_value(&imageUriJson);
            vfree(fullUri, vstrlen(fullUri)+1, MEMORY_TAG_STRING);
          }
        }
      }
    }
    free_json_value(&images);
  }

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
  out_gltf->bufferViews = vallocate(out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_RENDERER);
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
  out_gltf->accessors = vallocate(out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_RENDERER);
  for(int i = 0; i < out_gltf->accessorCount; i++){
    VEL_CHECK_MSG(extract_gltf_accessor(&accessors.data.array[i], &out_gltf->accessors[i], out_gltf), "GLTF File %s has malformed accessor object at index %d", uri, i);
  }
  free_json_value(&accessors);
  return TRUE;
}

void free_gltf_buffer(gltf_buffer* buf){
  vfree(buf->buffer, buf->size, MEMORY_TAG_RENDERER);
}

void free_gltf(gltf_object* out_gltf){
  for(int i = 0; i < out_gltf->bufferCount; i++){
    free_gltf_buffer(&out_gltf->buffers[i]);
  }
  vfree(out_gltf->buffers, out_gltf->bufferCount*sizeof(gltf_buffer), MEMORY_TAG_RENDERER);
  vfree(out_gltf->bufferViews, out_gltf->bufferViewCount*sizeof(gltf_buffer_view), MEMORY_TAG_RENDERER);
  for(int i = 0; i < out_gltf->accessorCount; i++){
    vfree(out_gltf->accessors[i].max, out_gltf->accessors->max_count*sizeof(gltf_value), MEMORY_TAG_RENDERER);
    vfree(out_gltf->accessors[i].min, out_gltf->accessors->min_count*sizeof(gltf_value), MEMORY_TAG_RENDERER);
    vfree(out_gltf->accessors[i].type, vstrlen(out_gltf->accessors[i].type)+1, MEMORY_TAG_RENDERER);
  }
  vfree(out_gltf->accessors, out_gltf->accessorCount*sizeof(gltf_accessor), MEMORY_TAG_RENDERER);
  vfree(out_gltf->images, out_gltf->imageCount*sizeof(velora_pixels), MEMORY_TAG_RENDERER);
}


void free_json_value(json_value *value){
  if(value->type == VELORA_JSON_STRING ||
     value->type == VELORA_JSON_OBJECT
  ){
    vfree(value->data.object, value->dataSize, MEMORY_TAG_JSON);
  }else if(value->type == VELORA_JSON_ARRAY){
    u64 len = value->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      free_json_value(&value->data.array[i]);
    }
    vfree(value->data.object, value->dataSize, MEMORY_TAG_JSON);
  }
}

void print_json_value(json_value *val){
  switch(val->type){
    case VELORA_JSON_DOUBLE:
    VINFO("Type: Double");
    VINFO("Data: %f", val->data);
    break;
    case VELORA_JSON_INTEGER:
    VINFO("Type: Integer");
    VINFO("Data: %d", val->data);
    break;
    case VELORA_JSON_STRING:
    VINFO("Type: String");
    VINFO("data: %s", val->data)
    break;
    case VELORA_JSON_OBJECT:
    VINFO("Type: Object");
    VINFO("Data: %s", val->data);
    break;
    case VELORA_JSON_ARRAY:
    VINFO("Type: Array");
    u64 len = val->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      print_json_value(&val->data.array[i]);
    }
    break;
  }
}