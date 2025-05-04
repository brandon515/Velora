#include "defines.h"
#include "core/stb_image.h"

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