#include "core/utils/vimage.h"
#include "core/stb_image.h"

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



