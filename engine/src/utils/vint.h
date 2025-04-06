#pragma once

#include "defines.h"

/*! 
 * @brief Clamps the value between the minimum and maximum
 * @param value An interger to be contained
 * @param minimum The minimum that value is allowed to be
 * @param maximum The maximum that value is allowed to be
 * @return value if the value is between minimum and maximum, minimum if value is smaller than minimum and likewise with maximum
 */
u32 vclamp(u32 value, u32 minimum, u32 maximum);