#pragma once

#include "defines.h"
#include "render/render_api.h"
#include "core/utils/vimage.h"
#include "core/utils/vgltf.h"

typedef struct _vmaterial {
  velora_pixels baseColor;
} vmaterial;

/*!
 * @brief Creates a material from the provided URI images
 * @param renderState The state of the render system
 * @param baseColorURI The URI to the base color image on the HDD
 * @param outMaterial The material to put the handles into
 * @return TRUE if the material was able to created, FALSE if there is no image at the provided URIs
 */
VRENDER_API b8 vmaterial_from_image(const char* baseColorURI, vmaterial *outMaterial);

VRENDER_API b8 vmaterial_from_gltf(gltf_object *gltf, u64 matIndex, vmaterial *outMaterial);

VRENDER_API void free_vmaterial(vmaterial* material);