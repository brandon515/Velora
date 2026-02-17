#include "defines.h"
#include "vmemory.h"
#include "asserts.h"

#define STBI_ASSERT(x) VASSERT_MSG(x, "STB_Image ran into an error")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"