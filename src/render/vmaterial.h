#pragma once

#include "defines.h"
#include "utils/vimage.h"
#include "velora_render.h"

typedef struct _vmaterial {
  u64 baseColorHandle;
} vmaterial;

/*!
 * @brief Creates a material from the provided URI images
 * @param renderState The state of the render system
 * @param baseColorURI The URI to the base color image on the HDD
 * @param outMaterial The material to put the handles into
 * @return TRUE if the material was able to created, FALSE if there is no image at the provided URIs
 */
b8 vmaterial_from_image(render_state *renderState, const char* baseColorURI, vmaterial *outMaterial);