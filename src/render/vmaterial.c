#include "vmaterial.h"

b8 vmaterial_from_image(const char* baseColorURI, vmaterial *outMaterial){
  velora_pixels baseColor;
  VEL_CHECK(import_pixels(baseColorURI, &baseColor));
  return TRUE;
}