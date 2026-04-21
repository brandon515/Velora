#include "vmaterial.h"

b8 vmaterial_from_image(render_state *renderState, const char* baseColorURI, vmaterial *outMaterial){
  velora_pixels baseColor;
  VEL_CHECK(import_pixels(baseColorURI, &baseColor));
  VEL_CHECK(register_texture(renderState, baseColor, &outMaterial->baseColorHandle));
  return TRUE;
}