#include "vfile.h"
#include "vstring.h"
#include <stdio.h>
#include "core/vmemory.h"

u64 get_file_size(FILE* file){
  u64 cur_pos = ftell(file);
  fseek(file, 0, SEEK_END);
  u64 ret_pos = ftell(file);
  fseek(file, cur_pos, SEEK_SET);
  return ret_pos;
}

b8 get_file_contents(const char* uri, velora_file* out_buffer){
  FILE* file = fopen(uri, "rb");
  if(file == NULL){
    return FALSE;
  }
  out_buffer->size = get_file_size(file);

  out_buffer->contents = vallocate(out_buffer->size, MEMORY_TAG_FILE);

  u64 bytesRead = fread(out_buffer->contents, sizeof(u8), out_buffer->size, file);
  fclose(file);
  if(bytesRead != out_buffer->size){
    return FALSE;
  }
  return TRUE;
}

void free_velora_file(velora_file *file){
  vfree(file->contents, file->size, MEMORY_TAG_FILE);
  file->size = 0;
}

char* get_file_path(const char* uri){
  u64 index = vrfind(uri, '/');
  if(index == U64_MAX){
    index = vrfind(uri, '\\');
    if(index == U64_MAX){
      return "./";
    }
  }
  return vsubstr(uri, 0, index+1);
}
