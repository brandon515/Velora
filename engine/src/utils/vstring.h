#pragma once

#include "defines.h"

/*! 
 * @brief Function takes the length of a 0 terminated string
 * @param str Pointer to a zero terminated string
 * @return Length of string in bytes minus the 0 termination
 */
VAPI u64 vstrlen(const char* str);

/*!
 * @brief Compares two 0 terminated strings
 * @param str1 First zero terminated string
 * @param str2 Second zero terminated string
 * @return TRUE if the strings are identical and the same length, FALSE otherwise
 */
VAPI u8 vstrcmp(const char* str1, const char* str2);