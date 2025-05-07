#pragma once
#include "defines.h"
#include <stdio.h>

/*!
 * @brief Gets the size of the file in bytes
 * @param file The handle to the file
 * @return The size of the file in bytes
 */
u64 get_file_size(FILE* file);

/*!
 * @brief Outputs the full file contents into out_buffer
 * @param file The handle to the file
 * @param out_buffer A pointer to an allocated buffer to output into
 * @return FALSE if the file is NULL or if the amount of data read isn't the whole file, TRUE otherwise
 */
b8 get_file_contents(FILE* file, u8* out_buffer);

/**
 * @brief Get the path of the URI and excludes the final file name
 * @param uri A zero terminated string with the full URI
 * @return A heap allocated zero-terminated string with the path or NULL if the uri is just a file name
 */
char* get_file_path(const char* uri);