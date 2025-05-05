#include "vimport.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "utils/vstring.h"
#include <stdlib.h>

b8 import_pixels(const char *uri, velora_pixels *out_pixels){
  int height, width, chans;
  stbi_uc* pixels = stbi_load(uri, &width, &height, &chans, STBI_rgb_alpha);
  if(!pixels){
    return FALSE;
  }
  out_pixels->width = width;
  out_pixels->height = height;
  // the reason we include 4 here is becuase STBI_rgb_alpha forces the pixels to be 32 bit
  out_pixels->size = out_pixels->width*out_pixels->height*4;
  out_pixels->pixels = vallocate(out_pixels->size, MEMORY_TAG_IMAGE);
  vcopy_memory(out_pixels->pixels, pixels, out_pixels->size);
  stbi_image_free(pixels);
  return TRUE;
}

void free_pixels(velora_pixels *pixels){
  vfree(pixels->pixels, pixels->size, MEMORY_TAG_IMAGE);
}

b8 does_json_name_match(u8* data, const char* name){
  if(data[0] != '"'){
    VERROR("Did not start at a quotation mark");
  }
  data++; //increment the data past the quotation mark
  u64 stringSize = 0;
  while(data[stringSize] != '"'){
    stringSize++;
  }
  char string[stringSize+1];
  vcopy_memory(string, data+1, stringSize);
  string[stringSize] = 0;
  return vstrcmp(name, string);
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
  out_object->data.object = vallocate(jsonDataSize+1, MEMORY_TAG_JSON);
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
  while(data[numberLength] != ',' && data[numberLength] != '}'){
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
  out_object->data.string = vallocate(valueSize+1, MEMORY_TAG_JSON);
  out_object->dataSize = valueSize+1;
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
  extract_json_value(data, &out_object->data.array[arrayIt]);
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
      extract_json_value(data, &out_object->data.array[arrayIt]);
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