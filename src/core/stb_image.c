#include "defines.h"
#include "vmemory.h"
#include "asserts.h"

void *stb_vallocate(u64 size){
  u64 *mem = vallocate(size+sizeof(u64), MEMORY_TAG_IMAGE);
  mem[0] = size;
  return mem+1;
}

void *stb_vreallocate(void *block, u64 new_size){
  u64 *u64_block = ((u64*)block)-1;
  u64 old_size = u64_block[0];
  void* new_ptr = vreallocate(block, old_size, new_size+sizeof(u64), MEMORY_TAG_IMAGE);
  if(new_ptr == NULL){
    return NULL;
  }
  u64 *new_block = (u64*)new_ptr;
  new_block[0] = new_size;
  return new_block+1;
}

void stb_vfree(void *block){
  u64 *u64_block = ((u64*)block)-1;
  u64 size = u64_block[0];
  return vfree(u64_block, size, MEMORY_TAG_IMAGE);
}

#define STBI_ASSERT(x) VASSERT_MSG(x, "STB_Image ran into an error")
#define STBI_MALLOC(sz)           stb_vallocate(sz)
#define STBI_REALLOC(p,newsz)     stb_vreallocate(p,newsz)
#define STBI_FREE(p)              stb_vfree(p)

#define STBI_ASSERT(x) VASSERT_MSG(x, "STB_Image ran into an error")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"