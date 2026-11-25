#include "defines.h"
#if VPLATFORM_WINDOWS
  #define vftell(f)        _ftelli64(f)
  #define vfseek(f, o, w)  _fseeki64((f), (o), (w))
#else
  #define vftell(f)        ftello(f)
  #define vfseek(f, o, w)  fseeko((f), (o), (w))
#endif

#include "core/utils/vfile.h"
#include "core/utils/vstring.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "core/memory/vmemory.h"
#include "core/logger.h"

static b8 get_file_size(FILE* file, u64 *out_size){
  i64 cur_pos = vftell(file);
  if(cur_pos == -1){
    VERROR("Failed to get current position of file cursor: %s", strerror(errno));
    return FALSE;
  }
  if(vfseek(file, 0, SEEK_END) != 0){
    VERROR("Failed to seek to end of file: %s", strerror(errno));
    return FALSE;
  }
  i64 ret_pos = vftell(file);
  if(ret_pos == -1){
    VERROR("Failed to get position of cursor at end of file: %s", strerror(errno));
    return FALSE;
  }
  if(vfseek(file, cur_pos, SEEK_SET)){
    VERROR("Failed to reset position in file after seeking to end: %s", strerror(errno));
    return FALSE;
  }
  (*out_size) = ret_pos;
  return TRUE;
}

static b8 open_file(const char* uri, FILE** out_file, u64* out_size){
  FILE* file = fopen(uri, "rb");
  if(file == NULL){
    int err = errno;
    VERROR("Failed to open file '%s': %s", uri, strerror(err));
    return FALSE;
  }
  if(!get_file_size(file, out_size)){
    if(fclose(file) != 0){
      VWARN("Unable to close file %s", uri);
    }
    return FALSE;
  }
  (*out_file) = file;
  return TRUE;
}

static b8 dump_file_contents(FILE* file, u64 size,  u8* out_buffer){
  u64 bytesRead = fread(out_buffer, sizeof(u8), size, file);
  if(bytesRead != size){
    VERROR("Entire file was not read, %llu bytes read of a %llu byte file", bytesRead, size);
    return FALSE;
  }
  return TRUE;
}

static b8 import_file_internal(const char* uri, u64 extra_bytes, u64* out_size, u8** out_buffer){
  FILE* file;
  u64 file_size = 0;
  VEL_CHECK(open_file(uri, &file, &file_size));
  (*out_size) = file_size+extra_bytes;
  (*out_buffer) = vallocate((*out_size), MEMORY_TAG_FILE);

  b8 retVal = dump_file_contents(file, file_size, (*out_buffer));
  
  if(fclose(file) != 0){
    VWARN("Unable to close file %s", uri);
  }

  if(retVal == FALSE){
    vfree((*out_buffer), (*out_size), MEMORY_TAG_FILE);
    (*out_buffer) = NULL;
    (*out_size) = 0;
  }

  return retVal;
}

b8 get_file_contents(const char* uri, velora_file* out_buffer){
  VEL_CHECK(import_file_internal(uri, 0, &out_buffer->size, &out_buffer->contents));
  return TRUE;
}

b8 get_file_text(const char* uri, velora_file* out_buffer){
  VEL_CHECK(import_file_internal(uri, 1, &out_buffer->size, &out_buffer->contents));
  out_buffer->contents[out_buffer->size-1] = '\0';
  return TRUE;
}

void free_velora_file(velora_file *file){
  vfree(file->contents, file->size, MEMORY_TAG_FILE);
  file->size = 0;
  file->contents = NULL;
}

char* get_file_path(const char* uri){
  u64 index = vrfind(uri, '/');
  if(index == U64_MAX){
    index = vrfind(uri, '\\');
    if(index == U64_MAX){
      char* ret = vallocate(3, MEMORY_TAG_STRING);
      vcopy_memory(ret, "./", 3);
      return ret;
    }
  }
  return vsubstr(uri, 0, index+1);
}
