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

char* vconcat(const char *str1, const char* str2){
  u64 str1Len = vstrlen(str1);
  u64 str2Len = vstrlen(str2);
  u64 stringLength = str1Len+str2Len;
  u64 memorySize = stringLength + 1; //account for the 0 termination
  char* buf = vallocate(memorySize, MEMORY_TAG_STRING);
  vcopy_memory(buf, str1, str1Len);
  vcopy_memory(buf+str1Len, str2, str2Len);
  buf[stringLength] = 0;
  return buf;
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

b8 vstrtoint(const char *str, i64 *out_int){
  u64 index = 0;
  i64 negativeMul = 1;
  if(str[index] == '-'){
    index++;
    negativeMul = -1;
  }
  if(str[index] < '0' || str[index] > '9'){
    return FALSE;
  }
  while(str[index] >= '0' && str[index] <= '9'){
    index++;
  }
  u64 place = 1;
  for(int i = index; i >= 0; i--){
    if(str[index] >= '0' && str[index] <= '9'){
      (*out_int) += (str[i] - '0')*place;
    }
  }
  (*out_int) *= negativeMul;
  return TRUE;
}

b8 vinttostr(i64 in, char **out_str){
  i64 scratchValue = in;
  u64 digitLength = 0;
  if(scratchValue < 0){
    scratchValue *= -1;
    digitLength++;
  }
  while(scratchValue > 0){
    scratchValue/=10;
    digitLength++;
  }
  (*out_str) = vallocate(digitLength+1, MEMORY_TAG_STRING);
  (*out_str)[digitLength] = 0;
  scratchValue = in;
  if(scratchValue < 0){
    (*out_str)[0] = '-';
    scratchValue *= -1;
  }
  digitLength--;
  while(scratchValue > 0){
    (*out_str)[digitLength] = (scratchValue%10)+'0';
    scratchValue /= 10;
    digitLength--;
  }
  return TRUE;
}
