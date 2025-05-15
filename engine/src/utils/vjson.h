#pragma once
#include "defines.h"
#include "core/logger.h"

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

typedef enum _json_type{
  VELORA_JSON_INTEGER,
  VELORA_JSON_DOUBLE,
  VELORA_JSON_STRING,
  VELORA_JSON_OBJECT,
  VELORA_JSON_ARRAY,
}json_type;

typedef struct _json_value json_value;

typedef union _json_data{
  i64 integer;
  f64 dFloat;
  char* string;
  u8* object;
  json_value* array;
}json_data;

typedef struct _json_value{
  json_type type; // The type of value from the json file
  json_data data; // a union of all the data types a json variable could possibly be
  u64 dataSize; // datasize for the pointer data types, this is unused for integer and doubles
}json_value;


/**
 * @brief Pulls a value out of the raw data from a JSON object
 * @param data The data for a valid JSON object
 * @param name The variable name that needs to be pulled
 * @param out_object A pointer to a json_value that will be filled with the requested data
 * @return FALSE if the variable doesn't exist or the data doesn't contain a JSON object, TRUE otherwise
 */
VAPI b8 get_json_value(u8* data, const char *name, json_value *out_object);

/**
 * @brief Frees the values inside the json_value struct if it needs to be freed
 * @param value a json_value that was created with get_json_value
 */
VAPI void free_json_value(json_value *value);

/**
 * @brief Prints out the value using VINFO
 * @param val The json_value to print out
 */
VAPI void print_json_value(json_value *val);