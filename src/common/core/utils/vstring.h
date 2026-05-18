#pragma once

#include "defines.h"

/*! 
 * @brief Function takes the length of a 0 terminated string
 * @param str Pointer to a zero terminated string
 * @return Length of string in bytes minus the 0 termination
 */
 u64 vstrlen(const char* str);

/*!
 * @brief Compares two 0 terminated strings
 * @param str1 First zero terminated string
 * @param str2 Second zero terminated string
 * @return TRUE if the strings are identical and the same length, FALSE otherwise
 */
 u8 vstrcmp(const char* str1, const char* str2);

/**
 * @brief Creates a substring that is [startIndex, startIndex+length] of str array
 * @param str A pointer to a zero-terminated char array
 * @param startIndex The position of the first letter to include in the substring
 * @param length The length of the desired substring, not including the zero at the end of the string
 * @return A pointer to a heap allocated zero-terminated char array or NULL if startIndex or startIndex+length is beyond the length of the string
 */
 char* vsubstr(const char* str, u64 startIndex, u64 length);

/**
 * @brief Finds the last instance of the character provided
 * @param str A pointer to a zero-terminated char array
 * @param character The character to search for
 * @return a u64 with the index of the character in the string or U64_MAX if none was found
 */
 u64 vrfind(const char* str, char character);

/**
 * @brief Finds the first instance of the character provided
 * @param str A pointer to a zero-terminated char array
 * @param character The character to search for
 * @return a u64 with the index of the character in the string or U64_MAX if none was found
 */
 u64 vfind(const char* str, char character);

/**
 * @brief Concatenate two strings into a third heap allocated string
 * @param str1 A pointer to a const char array that will provide the first half of the string
 * @param str2 A pointer to a const char array that will provide the second half of the string
 * @return A pointer to a heap allocated string with data copied from str1 and str2
 */
 char* vconcat(const char *str1, const char* str2);

 b8 vstrtoint(const char *str, i64 *out_int);
 b8 vinttostr(i64 in, char **out_str);
