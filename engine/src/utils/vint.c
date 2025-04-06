#include "vint.h"

u32 vclamp(u32 value, u32 minimum, u32 maximum){
  if(value < minimum){
    return minimum;
  }else if(value > maximum){
    return maximum;
  }else{
    return value;
  }
}
