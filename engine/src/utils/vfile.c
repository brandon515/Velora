#include "vfile.h"
#include "vstring.h"

u64 get_file_size(FILE* file){
  u64 cur_pos = ftell(file);
  fseek(file, 0, SEEK_END);
  u64 ret_pos = ftell(file);
  fseek(file, cur_pos, SEEK_SET);
  return ret_pos;
}

b8 get_file_contents(FILE* file, u8* out_buffer){
  if(file == NULL){
    return FALSE;
  }
  u64 fileSize = get_file_size(file);

  u64 bytesRead = fread(out_buffer, sizeof(u8), fileSize, file);
  if(bytesRead != fileSize){
    return FALSE;
  }
  return TRUE;
}

char* get_file_path(const char* uri){
  u64 index = vrfind(uri, '/');
  if(index == U64_MAX){
    index = vrfind(uri, '\\');
    if(index == U64_MAX){
      return NULL;
    }
  }
  return vsubstr(uri, 0, index+1);
}
