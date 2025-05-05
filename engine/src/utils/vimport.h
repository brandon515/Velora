#include "defines.h"
#include "core/stb_image.h"
#include "container/darray.h"

typedef struct _velora_pixels{
  u8* pixels;
  u64 width;
  u64 height;
  u64 size;
}velora_pixels;

typedef struct _gltf_buffer{
  u64 size;
  u8* buffer;
}gltf_buffer;

typedef struct _gltf_object{
  gltf_buffer *buffers;
  u64 bufferCount;
}gltf_object;

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
 * @brief Imports an image with the provided URI
 * @param uri a string pointing to the filepath for the image to import
 * @param out_pixels a pointer to a velora_pixels struct
 * @return returns FALSE if image doesn't exist, TRUE otherwise
 */
b8 import_pixels(const char *uri, velora_pixels *out_pixels);

/**
 * @brief frees the memory allocated in import_pixels
 * @param out_pixels a pointer to the velora_pixels that was previously allocated by import_pixels
 */
void free_pixels(velora_pixels *pixels);

/**
 * @brief imports the gltf file into a usable file
 * @param uri The filename of the gltf file
 * @return A pointer to the usable gltf_object
 */
gltf_object* import_gltf(const char *uri);


b8 get_json_value(u8* data, const char *name, json_value *out_object);

void free_json_value(json_value *value);

void print_json_value(json_value *val);