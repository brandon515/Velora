#pragma once
#include "defines.h"

typedef struct _velora_file{
  u64 size;
  u8* contents;
} velora_file;

/*!
 * @brief Outputs the full file contents into out_buffer
 * @param uri A pointer to a const char array containing the file path to the file
 * @param out_buffer A pointer to a velora_file object to be filled
 * @return FALSE if the file doesn't exist or if the amount of data read isn't the whole file, TRUE otherwise
 */
b8 get_file_contents(const char* uri, velora_file* out_buffer);

/**
 * @brief Get the path of the URI and excludes the final file name
 * @param uri A zero terminated string with the full URI
 * @return A heap allocated zero-terminated string with the path or NULL if the uri is just a file name
 */
char* get_file_path(const char* uri);