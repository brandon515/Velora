#pragma once
#include "defines.h"
#include "core/logger.h"

typedef enum _json_type{
  VELORA_JSON_NULL,
  VELORA_JSON_BOOL,
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
  b8 boolean;
  json_value* objectArray;
  json_value* array;
}json_data;

typedef struct _json_value{
  char *name;
  json_type type; // The type of value from the json file
  json_data data; // a union of all the data types a json variable could possibly be
  u64 dataSize; // datasize for the pointer data types, this is unused for integer and doubles
}json_value;


/**
 * @brief Frees the values inside the json_value struct if it needs to be freed, this is only to be used on json_value gotten from import_json_file
 * @param value a json_value that was created with get_json_value
 */
VAPI void free_json_value(json_value *value);


/**
 * @brief Imports the data from the uri provided and puts it into the json_value
 * @param uri The file path to the Json file
 * @param out_value a pointer to the json_value that needs to be filled
 * @return FALSE if the json file is malformed in some way or if it's not able to be found, TRUE otherwise
 */
VAPI b8 import_json_file(const char *uri, json_value *out_value);

/**
 * @brief Prints out the value using VINFO
 * @param val The json_value to print out
 */
VAPI void print_json_value(json_value *val);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to store the integer value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_unsigned_integer(json_value* obj, const char *name, u64 *value);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to store the integer value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, any pointer is NULL, or the value is less than 0. TRUE otherwise
 */
VAPI b8 load_json_signed_integer(json_value* obj, const char *name, i64 *value);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value A pointer to where the string lives in memory, the string will be allocated in this function
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_STRING, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_string(json_value* obj, const char *name, char **value);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to store the float value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_float(json_value* obj, const char *name, f64 *value);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to store the json object value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_object(json_value* obj, const char *name, json_value *value);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to create the array, The variable will be allocated in this function
 * @param count The number of u64 variables in value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_unsigned_integer_array(json_value* obj, const char *name, u64 **value, u64 *count);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to create the array, The variable will be allocated in this function
 * @param count The number of u64 variables in value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, any pointer is NULL, or the number is less than 0. TRUE otherwise
 */
VAPI b8 load_json_signed_integer_array(json_value* obj, const char *name, i64 **value, u64 *count);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to create the array, The variable will be allocated in this function
 * @param count The number of f64 variables in value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_float_array(json_value* obj, const char *name, f64 **value, u64 *count);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to create the array, The variable will be allocated in this function as will the strings
 * @param count The number of zero-terminated strings in value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_string_array(json_value* obj, const char *name, char ***value, u64 *count);

/**
 * @brief Loads the json value into the provided variable
 * @param obj A json_value that uses the object type in the union
 * @param name The name of the variable in the json object
 * @param value The pointer to where to create the array, the variable will be allocated in this function
 * @param count The number of json_values in value
 * @return FALSE if obj isn't a VELORA_JSON_OBJECT type, the retrived value isn't VELORA_JSON_INTEGER, or if any pointer is NULL. TRUE otherwise
 */
VAPI b8 load_json_object_array(json_value* obj, const char *name, json_value **value, u64 *count);