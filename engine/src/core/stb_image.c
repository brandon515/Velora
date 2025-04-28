#include "defines.h"
#include "vmemory.h"
#include "asserts.h"

void *stb_reallocate(void* oldPointer, u64 oldSize, u64 newSize){
  vfree(oldPointer, oldSize, MEMORY_TAG_IMAGE);
  return vallocate(newSize, MEMORY_TAG_IMAGE);
}

#define STBI_ASSERT(x) VASSERT_MSG(x, "STB_Image ran into an error")
#define STBI_MALLOC(sz) vallocate(sz, MEMORY_TAG_IMAGE)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) stb_reallocate(p, oldsz, newsz)
#define STBI_FREE(p) vfree(p, 0, MEMORY_TAG_IMAGE)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"