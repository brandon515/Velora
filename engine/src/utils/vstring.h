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

/**
 * @brief Creates a substring that is [startIndex, startIndex+length] of str array
 * @param str A pointer to a zero-terminated char array
 * @param startIndex The position of the first letter to include in the substring
 * @param length The length of the desired substring, not including the zero at the end of the string
 * @return A pointer to a heap allocated zero-terminated char array or NULL if startIndex or startIndex+length is beyond the length of the string
 */
VAPI char* vsubstr(const char* str, u64 startIndex, u64 length);

/**
 * @brief Finds the last instance of the character provided
 * @param str A pointer to a zero-terminated char array
 * @param character The character to search for
 * @return a u64 with the index of the character in the string or U64_MAX if none was found
 */
VAPI u64 vrfind(const char* str, char character);

/**
 * @brief Finds the first instance of the character provided
 * @param str A pointer to a zero-terminated char array
 * @param character The character to search for
 * @return a u64 with the index of the character in the string or U64_MAX if none was found
 */
VAPI u64 vfind(const char* str, char character);