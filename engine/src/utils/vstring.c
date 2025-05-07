#include "vstring.h"
#include "core/vmemory.h"

u64 vstrlen(const char* str){
  u64 len = 0;
  while((*str) != 0){
    len++;
    str++;
  }
  return len;
}

u8 vstrcmp(const char* str1, const char* str2){
  // Check the first character, if they're not equal we can short circuit
  if((*str1) != (*str2)){
    return FALSE;
  }
  while((*str1) != 0 && (*str2) != 0){
    /* We push the string forward by one so we can check the final 0 terminated
     * character. If str1 or str2 are equal but either one terminates early
     * than this will catch that.
     */
    str1++;
    str2++;
    if((*str1) != (*str2)){
      return FALSE;
    }
  }
  return TRUE;
}

char* vsubstr(const char* str, u64 startIndex, u64 length){
  u64 strLength = vstrlen(str);
  if(startIndex >= strLength || startIndex+length >= strLength){
    return NULL;
  }
  char* retStr = vallocate(length+1, MEMORY_TAG_STRING);
  vcopy_memory(retStr, str+startIndex, length);
  retStr[length] = 0;
  return retStr;
}

u64 vrfind(const char* str, char character){
  u64 strLength = vstrlen(str);
  for(int i = strLength; i >= 0; i--){
    if(str[i] == character){
      return i;
    }
  }
  return U64_MAX;
}

u64 vfind(const char* str, char character){
  u64 strLength = vstrlen(str);
  for(int i = 0; i <= strLength; i++){
    if(str[i] == character){
      return i;
    }
  }
  return U64_MAX;
}