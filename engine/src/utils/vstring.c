#include "vstring.h"

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