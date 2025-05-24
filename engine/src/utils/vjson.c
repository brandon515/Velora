#include "vjson.h"
#include "core/vmemory.h"
#include "core/logger.h"
#include "utils/vstring.h"
#include "container/darray.h"
#include <stdlib.h>

b8 is_number(char character){
  return (character >= 48) && (character <= 57);
}

b8 extract_json_object(u8* data, json_value *out_object){
  u64 jsonDataSize = 1;
  u64 workingLevel = 1;
  while(workingLevel >= 1){
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

b8 process_json_string(u8 *data, char** out_str, u64 *jsonStrLength){
  if((*data) !=  '"'){
    return FALSE;
  }
  u8 *lengthArray = data;
  lengthArray++;
  u64 byteLength = 0;
  while(TRUE){
    if((*lengthArray) == '"'){
      break;
    }
    if((*lengthArray) == '\\'){
      lengthArray++;
      if((*lengthArray) == 'u'){
        lengthArray = lengthArray+4;
      }
    }
    lengthArray++;
    byteLength++;
  }
  byteLength++; // Gotta add the zero termination
  (*jsonStrLength) = (lengthArray-data)+1; // data[jsonStrLength] is the character after the last "
  (*out_str) = vallocate(byteLength, MEMORY_TAG_STRING);
  char *str = (*out_str);
  u64 inputStrIndex = 0;
  data++;
  for(int i = 0; i < byteLength-1; i++){
    if(data[inputStrIndex] == '\\'){
      inputStrIndex++;
      switch(data[inputStrIndex]){
        case '"':
          str[i] = '"';
          break;
        case '\\':
          str[i] = '\\';
          break;
        case '/':
          str[i] = '/';
          break;
        case 'b':
          str[i] = '\b';
          break;
        case 'f':
          str[i] = '\f';
          break;
        case 'n':
          str[i] = '\n';
          break;
        case 'r':
          str[i] = '\r';
          break;
        case 't':
          str[i] = '\t';
          break;
        case 'u':
          inputStrIndex = inputStrIndex+4;
          str[i] = '?';
      }
    }else{
      str[i] = data[inputStrIndex];
    }
    inputStrIndex++;
  }
  str[byteLength-1] = 0;
  return TRUE;
}

b8 process_json_integer(u8 *data, i64 *out_value, u64 *jsonNumberLength){
  if((*data) != '-' && (*data) < '0' && (*data) > '9'){
    return FALSE;
  }
  (*jsonNumberLength) = 0;
  i64 negativeMul = 1;
  if((*data) == '-'){
    negativeMul = -1;
    (*jsonNumberLength)++;
  }
  while(data[(*jsonNumberLength)] >= '0' && data[(*jsonNumberLength)] <= '9'){
    (*jsonNumberLength)++;
  }
  (*out_value) = 0;
  u64 curPlace = 1;
  for(int i = (*jsonNumberLength); i >= 0; i--){
    if(data[i] >= '0' && data[i] <= '9'){
      (*out_value) += ((data[i]-'0') * curPlace);
      curPlace *= 10;
    }
  }
  (*out_value) *= negativeMul;
  return TRUE;
}

b8 process_json_float(u8 *data, f64 *out_value, u64 *jsonNumberLength){
  if((*data) != '-' && (*data) <= '0' && (*data) >= '9'){
    return FALSE;
  }
  char scratchStr[1024];
  u64 numberLen = 0;
  while((data[numberLen] >= '0' && data[numberLen] <= '9') || data[numberLen] == '.' || data[numberLen] == 'e' || data[numberLen] == 'E' || data[numberLen] == '-'){
    scratchStr[numberLen] = data[numberLen];
    numberLen++;
  }
  (*jsonNumberLength) = numberLen;
  scratchStr[numberLen] = 0;
  (*out_value) = atof(scratchStr);
  return TRUE; 
}

b8 process_json_bool(u8 *data, b8 *out_value, u64 *jsonLength){
  if((*data) != 't' && (*data) != 'f'){
    return FALSE;
  }
  if((*data) == 't'){
    (*out_value) = TRUE;
  }else{
    (*out_value) = FALSE;
  }
  (*jsonLength) = 0;
  while(data[(*jsonLength)] >= 'a' && data[(*jsonLength)] <= 'z'){
    (*jsonLength)++;
  }
  return TRUE;
}

b8 identify_json_value(u8* data, json_type *out_type){
  if((*data) == '"'){
    (*out_type) = VELORA_JSON_STRING;
  }else if((*data) == '['){
    (*out_type) = VELORA_JSON_ARRAY;
  }else if((*data) == '{'){
    (*out_type) = VELORA_JSON_OBJECT;
  }else if((*data) == 't' || (*data) == 'f'){
    (*out_type) = VELORA_JSON_BOOL;
  }else if((*data) == '-' || ((*data) >= '0' && (*data) <= '9')){
    u64 index = 0;
    if(data[index] == '-'){
      index++;
    }
    while(data[index] >= '0' && data[index] <= '9'){
      index++;
    }
    if(data[index] == '.' || data[index] == 'e' || data[index] == 'E'){
      (*out_type) = VELORA_JSON_DOUBLE;
    }else{
      (*out_type) = VELORA_JSON_INTEGER;
    }
  }else if((*data) == 'n'){
    (*out_type) = VELORA_JSON_NULL;
  }else{
    return FALSE;
  }
  return TRUE;
}

b8 process_json_array(u8* data, json_value **out_value, u64 *jsonLength){
  if((*data) != '['){
    return FALSE;
  }
  u64 index = 1; // start after the opening bracket
  while(data[index] < ' '){
    index++;
  }
  //darray *jsonValues = darray_new(sizeof(json_value));
  while(data[index] != ']'){
    //json_value arrayElem = {0};
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
  u64 inArray = 0;
  while(level > 0){
    u8 character = (*local_array);
    if((character == '{' || character == ':') && inArray == 0){
      level++; 
    }else if(character == '['){
      inArray++;
      level++;
    }else if(character == ']'){
      inArray--;
      level--;
    }else if((character == '}' || character == ',') && inArray == 0){
      level--;
    }else if(character == '"' && level == 1 && inArray == 0){
      // See if this is the value we're trying to pull
      u64 stringSize = 0;
      local_array++;
      while(local_array[stringSize] != '"'){
        stringSize++;
      }
      char jsonName[stringSize+1];
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
    vfree(value->data.object, value->dataSize, MEMORY_TAG_JSON);
  }
}

void print_json_value(json_value *val){
  VINFO("Name: %s", val->name);
  switch(val->type){
    case VELORA_JSON_BOOL:
    VINFO("Type: Boolean");
    if(val->data.boolean == TRUE){
      VINFO("Data: TRUE");
    }else if(val->data.boolean == FALSE){
      VINFO("Data: FALSE");
    }
    break;
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
    VINFO("data: %s", val->data);
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
    case VELORA_JSON_NULL:
    VINFO("Type: NULL");
    break;
  }
}

b8 load_json_signed_integer(json_value* obj, const char *name, i64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_INTEGER){
    return FALSE;
  }
  (*value) = dummy.data.integer;
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_unsigned_integer(json_value* obj, const char *name, u64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_INTEGER){
    return FALSE;
  }
  if(dummy.data.integer < 0){
    return FALSE;
  }
  (*value) = dummy.data.integer;
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_string(json_value* obj, const char *name, char **value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_STRING){
    return FALSE;
  }
  (*value) = vallocate(dummy.dataSize, MEMORY_TAG_STRING);
  vcopy_memory((*value), dummy.data.string, dummy.dataSize);
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_float(json_value* obj, const char *name, f64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_DOUBLE){
    return FALSE;
  }
  (*value) = dummy.data.dFloat;
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_object(json_value* obj, const char *name, json_value *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  if(get_json_value(obj->data.object, name, value) == FALSE || value->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  return TRUE;
}

b8 load_json_signed_integer_array(json_value* obj, const char *name, i64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_ARRAY){
    return FALSE;
  }
  (*count) = dummy.dataSize/sizeof(json_value);
  (*value) = vallocate((*count)*sizeof(u64), MEMORY_TAG_JSON);
  i64 *valueArray = (*value);
  for(int i = 0; i < (*count); i++){
    if(dummy.data.array[i].type != VELORA_JSON_INTEGER){
      return FALSE;
    }
    valueArray[i] = dummy.data.array[i].data.integer;
  }
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_unsigned_integer_array(json_value* obj, const char *name, u64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_ARRAY){
    return FALSE;
  }
  (*count) = dummy.dataSize/sizeof(json_value);
  (*value) = vallocate((*count)*sizeof(u64), MEMORY_TAG_JSON);
  u64 *valueArray = (*value);
  for(int i = 0; i < (*count); i++){
    if(dummy.data.array[i].type != VELORA_JSON_INTEGER || dummy.data.array[i].data.integer < 0){
      return FALSE;
    }
    valueArray[i] = dummy.data.array[i].data.integer;
  }
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_float_array(json_value* obj, const char *name, f64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_ARRAY){
    return FALSE;
  }
  (*count) = dummy.dataSize/sizeof(json_value);
  (*value) = vallocate((*count)*sizeof(f64), MEMORY_TAG_JSON);
  f64 *valueArray = (*value);
  for(int i = 0; i < (*count); i++){
    if(dummy.data.array[i].type != VELORA_JSON_DOUBLE){
      return FALSE;
    }
    valueArray[i] = dummy.data.array[i].data.dFloat;
  }
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_string_array(json_value* obj, const char *name, char ***value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_ARRAY){
    return FALSE;
  }
  (*count) = dummy.dataSize/sizeof(json_value);
  (*value) = vallocate((*count)*sizeof(char*), MEMORY_TAG_JSON);
  char **valueArray = (*value);
  for(int i = 0; i < (*count); i++){
    if(dummy.data.array[i].type != VELORA_JSON_STRING){
      return FALSE;
    }
    valueArray[i] = vallocate(dummy.data.array[i].dataSize, MEMORY_TAG_STRING);
    vcopy_memory(valueArray[i], dummy.data.array[i].data.string, dummy.data.array[i].dataSize);
  }
  free_json_value(&dummy);
  return TRUE;
}

b8 load_json_object_array(json_value* obj, const char *name, json_value **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  json_value dummy = {0};
  if(get_json_value(obj->data.object, name, &dummy) == FALSE || dummy.type != VELORA_JSON_ARRAY){
    return FALSE;
  }
  (*count) = dummy.dataSize/sizeof(json_value);
  (*value) = vallocate(dummy.dataSize, MEMORY_TAG_JSON);
  json_value *valueArray = (*value);
  for(int i = 0; i < (*count); i++){
    if(dummy.data.array[i].type != VELORA_JSON_OBJECT){
      return FALSE;
    }
    valueArray[i].data.object = vallocate(dummy.data.array[i].dataSize, MEMORY_TAG_JSON);
    valueArray[i].dataSize = dummy.data.array[i].dataSize;
    valueArray[i].type = dummy.data.array[i].type;
    vcopy_memory(valueArray[i].data.object, dummy.data.array[i].data.object, dummy.data.array[i].dataSize);
  }
  free_json_value(&dummy);
  return TRUE;
}
