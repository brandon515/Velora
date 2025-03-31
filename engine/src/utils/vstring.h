#pragma once

#include "defines.h"

/* Function takes the length of a 0 terminated string
 * @param Pointer to a zero terminated string
 * @return Length of string in bytes minus the 0 termination
 */
VAPI u64 vstrlen(const char* str);

/* Compares two 0 terminated strings
 * @param First zero terminated string
 * @param Second zero terminated string
 * @return TRUE if the strings are identical and the same length, FALSE otherwise
 */
VAPI u8 vstrcmp(const char* str1, const char* str2);