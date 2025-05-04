#include "vimport.h"
#include "core/vmemory.h"

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