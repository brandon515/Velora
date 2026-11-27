#include "assets/vjson.h"
#include "core/memory/vmemory.h"
#include "core/logger.h"
#include "core/utils/vstring.h"
#include "core/utils/vfile.h"
#include "core/container/darray.h"
#include <stdlib.h>

b8 process_json_string(u8 *data, char** out_str, u64 *jsonStrLength){
  if((*data) !=  '"'){
    return FALSE;
  }
  u8 *lengthArray = data;
  lengthArray++;
  u64 byteLength = 0;
  b8 stringTerminated = FALSE;
  while((*lengthArray) != '\0'){
    if((*lengthArray) == '"'){
      stringTerminated = TRUE;
      break;
    }
    if((*lengthArray) == '\\'){
      lengthArray++;
      if((*lengthArray) == 'u'){
        for(int i = 0; i < 4; i++){
          lengthArray++;
          if((*lengthArray) == '\0'){
            VERROR("Unclosed string in json file: %s", data);
            return FALSE;
          }
        }
      }
    }
    lengthArray++;
    byteLength++;
  }
  if(stringTerminated == FALSE){
    VERROR("JSON string is not terminated with \" and therefore not valid");
    return FALSE;
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
          break;
        default:
          VERROR("Invalid escape character %c used", data[inputStrIndex]);
          vfree((*out_str), byteLength, MEMORY_TAG_STRING);
          (*out_str) = NULL;
          return FALSE;
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
  if((*data) != '-' && ((*data) < '0' || (*data) > '9')){
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
  if((*data) != '-' && ((*data) < '0' || (*data) > '9')){
    return FALSE;
  }
  char scratchStr[1024];
  u64 numberLen = 0;
  while((data[numberLen] >= '0' && data[numberLen] <= '9') || data[numberLen] == '.' || data[numberLen] == 'e' || data[numberLen] == 'E' || data[numberLen] == '-'){
    scratchStr[numberLen] = data[numberLen];
    numberLen++;
    if((data[numberLen-1] == 'e' || data[numberLen-1] == 'E') && data[numberLen] == '+'){
      scratchStr[numberLen] = data[numberLen];
      numberLen++;
    }
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
    (*out_type) = VELORA_JSON_INTEGER;
    u64 index = 0;
    if(data[index] == '-'){
      index++;
    }
    while(data[index] >= '0' && data[index] <= '9'){
      index++;
    }
    if(data[index] == '.' || data[index] == 'e' || data[index] == 'E'){
      (*out_type) = VELORA_JSON_DOUBLE;
    }
  }else if((*data) == 'n'){
    (*out_type) = VELORA_JSON_NULL;
  }else{
    return FALSE;
  }
  return TRUE;
}

void free_json_value_darray(darray* jsonValues){
  iterator it = darray_create_iterator(jsonValues);
  json_value *curValue;
  while(iterator_next(&it, (void**)&curValue)){
    free_json_value(curValue);
  }
  darray_free(jsonValues);
}

b8 process_json_object(u8 *data, json_value **out_value, u64 *jsonLength, u64 *outNumOfItems);

b8 process_json_array(u8* data, json_value **out_value, u64 *jsonLength, u64 *outNumOfItems){
  if((*data) != '['){
    return FALSE;
  }
  u64 index = 1; // start after the opening bracket
  while(data[index] <= ' '){
    index++;
  }
  if(data[index] == ']'){
    (*out_value) = NULL;
    (*jsonLength) = index+1;
    (*outNumOfItems) = 0;

    return TRUE;
  }
  darray *jsonValues = darray_new(sizeof(json_value));
  while(data[index] != ']'){
    json_value arrayElem = {0};
    if(vinttostr((i64)jsonValues->length, &arrayElem.name) == FALSE){
      free_json_value_darray(jsonValues);
      return FALSE;
    }
    while(data[index] <= ' '){
      index++;
    }
    if(identify_json_value(&data[index], &arrayElem.type) == FALSE){
      free_json_value(&arrayElem);
      free_json_value_darray(jsonValues);
      return FALSE;
    }
    u64 bytesToIncreaseIndex = 0;
    switch(arrayElem.type){
      case VELORA_JSON_BOOL:
      if(process_json_bool(&data[index], &arrayElem.data.boolean, &bytesToIncreaseIndex) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_DOUBLE:
      if(process_json_float(&data[index], &arrayElem.data.dFloat, &bytesToIncreaseIndex) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_INTEGER:
      if(process_json_integer(&data[index], &arrayElem.data.integer, &bytesToIncreaseIndex) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_STRING:
      if(process_json_string(&data[index], &arrayElem.data.string, &bytesToIncreaseIndex) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      arrayElem.dataSize = vstrlen(arrayElem.data.string)+1;
      break;
      case VELORA_JSON_NULL:{
        if(data[index] == 'n' && data[index+1] == 'u' && data[index+2] == 'l' && data[index+3] == 'l'){
          arrayElem.data.array = NULL;
          arrayElem.dataSize = 0;
          bytesToIncreaseIndex = 4;
        }else{
          VERROR("Malformed value, expected null");
          free_json_value(&arrayElem);
          free_json_value_darray(jsonValues);
          return FALSE;
        }
        break;
      }
      case VELORA_JSON_ARRAY:
      if(process_json_array(&data[index], &arrayElem.data.array, &bytesToIncreaseIndex, &arrayElem.dataSize) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      arrayElem.dataSize *= sizeof(json_value);
      break;
      case VELORA_JSON_OBJECT:
      if(process_json_object(&data[index], &arrayElem.data.objectArray, &bytesToIncreaseIndex, &arrayElem.dataSize) == FALSE){
        free_json_value(&arrayElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      arrayElem.dataSize *= sizeof(json_value);
      break;
    }
    darray_push(jsonValues, &arrayElem);
    index += bytesToIncreaseIndex;
    while(data[index] == ',' || data[index] <= ' '){
      index++;
    }
  }
  (*jsonLength) = index+1;
  if(jsonValues->length != 0){
    (*outNumOfItems) = jsonValues->length;
    (*out_value) = vallocate(sizeof(json_value)*jsonValues->length, MEMORY_TAG_JSON);
    vcopy_memory((*out_value), jsonValues->data, sizeof(json_value)*jsonValues->length);
  }
  darray_free(jsonValues);
  return TRUE;
}

b8 process_json_object(u8 *data, json_value **out_value, u64 *jsonLength, u64 *outNumOfItems){
  if((*data) != '{'){
    return FALSE;
  }
  u64 index = 1;
  darray *jsonValues = darray_new(sizeof(json_value));
  while(data[index] != '"' && data[index] != '}'){
    index++;
  }
  while(data[index] != '}'){
    json_value objElem = {0};
    u64 amountToMove = 0;
    if(process_json_string(&data[index], &objElem.name, &amountToMove) == FALSE){
      free_json_value_darray(jsonValues);
      return FALSE;
    }
    index += amountToMove;
    while(data[index] != ':'){
      if(data[index] == '}' || data[index] == '\0'){
        free_json_value(&objElem);
        VERROR("Malformed JSON object, label with no value");
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      index++;
    }
    index++;
    while(data[index] <= ' '){
      if(data[index] == '\0'){
        free_json_value(&objElem);
        VERROR("Reached the end of the file while trying to process object.");
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      index++;
    }
    if(identify_json_value(&data[index], &objElem.type) == FALSE){
      free_json_value(&objElem);
      free_json_value_darray(jsonValues);
      return FALSE;
    }
    switch(objElem.type){
      case VELORA_JSON_BOOL:
      if(process_json_bool(&data[index], &objElem.data.boolean, &amountToMove) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_DOUBLE:
      if(process_json_float(&data[index], &objElem.data.dFloat, &amountToMove) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_INTEGER:
      if(process_json_integer(&data[index], &objElem.data.integer, &amountToMove) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      break;
      case VELORA_JSON_STRING:
      if(process_json_string(&data[index], &objElem.data.string, &amountToMove) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      objElem.dataSize = vstrlen(objElem.data.string)+1;
      break;
      case VELORA_JSON_NULL:{
        if(data[index] == 'n' && data[index+1] == 'u' && data[index+2] == 'l' && data[index+3] == 'l'){
          objElem.data.array = NULL;
          objElem.dataSize = 0;
          amountToMove = 4;
        }else{
          VERROR("Malformed value, expected null");
          free_json_value(&objElem);
          free_json_value_darray(jsonValues);
          return FALSE;
        }
        break;
      }
      case VELORA_JSON_ARRAY:
      if(process_json_array(&data[index], &objElem.data.array, &amountToMove, &objElem.dataSize) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      objElem.dataSize *= sizeof(json_value);
      break;
      case VELORA_JSON_OBJECT:
      if(process_json_object(&data[index], &objElem.data.objectArray, &amountToMove, &objElem.dataSize) == FALSE){
        free_json_value(&objElem);
        free_json_value_darray(jsonValues);
        return FALSE;
      }
      objElem.dataSize *= sizeof(json_value);
      break;
    }
    darray_push(jsonValues, &objElem);
    index += amountToMove;
    while(data[index] == ',' || data[index] <= ' '){
      index++;
    }
  }
  (*jsonLength) = index+1;
  if(jsonValues->length > 0){
    (*outNumOfItems) = jsonValues->length;
    (*out_value) = vallocate(sizeof(json_value)*jsonValues->length, MEMORY_TAG_JSON);
    vcopy_memory((*out_value), jsonValues->data, sizeof(json_value)*jsonValues->length);
  }
  darray_free(jsonValues);
  return TRUE;
}

b8 import_json_file(const char *uri, json_value *out_value){
  velora_file jsonFile = {0};
  VEL_CHECK(get_file_text(uri, &jsonFile));
  if(jsonFile.contents[0] == '\0'){
    VERROR("Attempting to process \'%s\' as JSON when the file is empty", uri);
    free_velora_file(&jsonFile);
    return FALSE;
  }
  u64 lastNonZeroCharIndex = jsonFile.size-1;
  while(jsonFile.contents[lastNonZeroCharIndex] <= ' '){
    lastNonZeroCharIndex--;
  }
  if(jsonFile.contents[lastNonZeroCharIndex] != '}'){
    free_velora_file(&jsonFile);
    return FALSE;
  }
  out_value->type = VELORA_JSON_OBJECT;
  u64 fileSize = 0;
  u64 numOfItems = 0;
  if(process_json_object(jsonFile.contents, &out_value->data.objectArray, &fileSize, &numOfItems) == FALSE){
    free_velora_file(&jsonFile);
    return FALSE;
  }
  free_velora_file(&jsonFile);
  out_value->dataSize = numOfItems*sizeof(json_value);
  const char *rootName = "ROOT";
  u64 strLen = vstrlen(rootName);
  out_value->name = vallocate(strLen+1, MEMORY_TAG_STRING);
  vcopy_memory(out_value->name, rootName, strLen);
  out_value->name[strLen] = 0;
  return TRUE;
}

void free_json_value(json_value *value){
  if(value->type == VELORA_JSON_STRING){
    vfree(value->data.string, value->dataSize, MEMORY_TAG_STRING);
  }else if(value->type == VELORA_JSON_ARRAY){
    u64 len = value->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      free_json_value(&value->data.array[i]);
    }
    vfree(value->data.array, value->dataSize, MEMORY_TAG_JSON);
  }else if(value->type == VELORA_JSON_OBJECT){
    u64 len = value->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      free_json_value(&value->data.objectArray[i]);
    }
    vfree(value->data.objectArray, value->dataSize, MEMORY_TAG_JSON);
  }
  if(value->name != NULL){
    vfree(value->name, vstrlen(value->name)+1, MEMORY_TAG_STRING);
    value->name = NULL;
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
    VINFO("Data: %f", val->data.dFloat);
    break;
    case VELORA_JSON_INTEGER:
    VINFO("Type: Integer");
    VINFO("Data: %lld", val->data.integer);
    break;
    case VELORA_JSON_STRING:
    VINFO("Type: String");
    VINFO("data: %s", val->data.string);
    break;
    case VELORA_JSON_OBJECT:{
    VINFO("Type: Object");
    u64 len = val->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      print_json_value(&val->data.objectArray[i]);
    }
    break;}
    case VELORA_JSON_ARRAY:{
    VINFO("Type: Array");
    u64 len = val->dataSize/sizeof(json_value);
    for(int i = 0; i < len; i++){
      print_json_value(&val->data.array[i]);
    }
    break;}
    case VELORA_JSON_NULL:
    VINFO("Type: NULL");
    break;
  }
}

b8 load_json_signed_integer(json_value* obj, const char *name, i64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_INTEGER && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      (*value) = obj->data.objectArray[i].data.integer;
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_unsigned_integer(json_value* obj, const char *name, u64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_INTEGER && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      (*value) = (u64)obj->data.objectArray[i].data.integer;
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_string(json_value* obj, const char *name, char **value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_STRING && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      (*value) = vallocate(obj->data.objectArray[i].dataSize, MEMORY_TAG_STRING);
      vcopy_memory((*value), obj->data.objectArray[i].data.string, obj->data.objectArray[i].dataSize);
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_float(json_value* obj, const char *name, f64 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      if(obj->data.objectArray[i].type == VELORA_JSON_DOUBLE){
        (*value) = obj->data.objectArray[i].data.dFloat;
        return TRUE;
      }else if(obj->data.objectArray[i].type == VELORA_JSON_INTEGER){
        (*value) = obj->data.objectArray[i].data.integer*1.0f;
        return TRUE;
      }else{
        return FALSE;
      }
    }
  }
  return FALSE;
}

b8 load_json_boolean(json_value* obj, const char *name, b8 *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_BOOL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_BOOL && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      (*value) = obj->data.objectArray[i].data.boolean;
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_object(json_value* obj, const char *name, json_value *value){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_OBJECT && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      vcopy_memory(value, &obj->data.objectArray[i], sizeof(json_value));
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_signed_integer_array(json_value* obj, const char *name, i64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    json_value *objElem = &obj->data.objectArray[i];
    if(objElem->type == VELORA_JSON_ARRAY && vstrcmp(objElem->name, name) == TRUE){
      u64 arrayLen = objElem->dataSize/sizeof(json_value);
      (*value) = vallocate(arrayLen*sizeof(i64), MEMORY_TAG_JSON);
      (*count) = arrayLen;
      for(int i = 0; i < arrayLen; i++){
        if(objElem->data.array[i].type != VELORA_JSON_INTEGER){
          vfree((*value), arrayLen*sizeof(i64), MEMORY_TAG_JSON);
          (*count) = 0;
          return FALSE;
        }
        (*value)[i] = objElem->data.array[i].data.integer;
      }
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_unsigned_integer_array(json_value* obj, const char *name, u64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    json_value *objElem = &obj->data.objectArray[i];
    if(objElem->type == VELORA_JSON_ARRAY && vstrcmp(objElem->name, name) == TRUE){
      u64 arrayLen = objElem->dataSize/sizeof(json_value);
      (*value) = vallocate(arrayLen*sizeof(u64), MEMORY_TAG_JSON);
      (*count) = arrayLen;
      for(int i = 0; i < arrayLen; i++){
        if(objElem->data.array[i].type != VELORA_JSON_INTEGER){
          vfree((*value), arrayLen*sizeof(u64), MEMORY_TAG_JSON);
          (*count) = 0;
          return FALSE;
        }
        (*value)[i] = (u64)objElem->data.array[i].data.integer;
      }
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_float_array(json_value* obj, const char *name, f64 **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    json_value *objElem = &obj->data.objectArray[i];
    if(objElem->type == VELORA_JSON_ARRAY && vstrcmp(objElem->name, name) == TRUE){
      u64 arrayLen = objElem->dataSize/sizeof(json_value);
      (*value) = vallocate(arrayLen*sizeof(f64), MEMORY_TAG_JSON);
      (*count) = arrayLen;
      for(int i = 0; i < arrayLen; i++){
        if(objElem->data.array[i].type == VELORA_JSON_DOUBLE){
          (*value)[i] = objElem->data.array[i].data.dFloat; 
        }else if(objElem->data.array[i].type == VELORA_JSON_INTEGER){
          (*value)[i] = (f64)objElem->data.array[i].data.integer;
        }else{
          vfree((*value), arrayLen*sizeof(f64), MEMORY_TAG_JSON);
          (*value) = NULL;
          (*count) = 0;
          return FALSE;
        }
      }
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_string_array(json_value* obj, const char *name, char ***value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    json_value *objElem = &obj->data.objectArray[i];
    if(objElem->type == VELORA_JSON_ARRAY && vstrcmp(objElem->name, name) == TRUE){
      u64 arrayLen = objElem->dataSize/sizeof(json_value);
      (*value) = vallocate(arrayLen*sizeof(char*), MEMORY_TAG_JSON);
      (*count) = arrayLen;
      for(int i = 0; i < arrayLen; i++){
        if(objElem->data.array[i].type != VELORA_JSON_STRING){
          for(int j = 0; j < i; j++){
            vfree((*value)[j], objElem->data.array[j].dataSize, MEMORY_TAG_STRING);
          }
          vfree((*value), arrayLen*sizeof(char*), MEMORY_TAG_JSON);
          (*count) = 0;
          return FALSE;
        }
        (*value)[i] = vallocate(objElem->data.array[i].dataSize, MEMORY_TAG_STRING);
        vcopy_memory((*value)[i], objElem->data.array[i].data.string, objElem->data.array[i].dataSize);
      }
      return TRUE;
    }
  }
  return FALSE;
}

b8 load_json_object_array(json_value* obj, const char *name, json_value **value, u64 *count){
  if(obj == NULL || name == NULL || value == NULL || obj->type != VELORA_JSON_OBJECT || count == NULL){
    return FALSE;
  }
  u64 len = obj->dataSize/sizeof(json_value);
  for(int i = 0; i < len; i++){
    if(obj->data.objectArray[i].type == VELORA_JSON_ARRAY && vstrcmp(obj->data.objectArray[i].name, name) == TRUE){
      json_value *arrayObj = &obj->data.objectArray[i];
      (*value) = arrayObj->data.array;
      (*count) = arrayObj->dataSize/sizeof(json_value);
      return TRUE;
    }
  }
  return FALSE;
}
