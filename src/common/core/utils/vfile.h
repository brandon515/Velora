#pragma once
#include "defines.h"
#include "core/core_api.h"

typedef struct _velora_file{
  u64 size;
  u8* contents;
} velora_file;

/*!
 * @brief Outputs the full file contents into out_buffer
 * @param uri A pointer to a const char array containing the file path to the file
 * @param out_buffer A pointer to a velora_file object to be filled, the contents are a binary match to the file
 * @return FALSE if the file doesn't exist or if the amount of data read isn't the whole file, TRUE otherwise
 */
VCORE_API b8 get_file_contents(const char* uri, velora_file* out_buffer);

/*!
 * @brief Outputs the full file contents into out_buffer
 * @param uri A pointer to a const char array containing the file path to the file
 * @param out_buffer A pointer to a velora_file object to be filled, the contents are zero terminated with the size being the byte file size + 1
 * @return FALSE if the file doesn't exist or if the amount of data read isn't the whole file, TRUE otherwise
 */
VCORE_API b8 get_file_text(const char* uri, velora_file* out_buffer);

/**
 * @brief Frees the content variable and sets the size to 0
 * @param file A pointer to a velora_file object previously created through get_file_contents or get_file_text
 */
VCORE_API void free_velora_file(velora_file *file);

/**
 * @brief Get the path of the URI and excludes the final file name
 * @param uri A zero terminated string with the full URI
 * @return A heap allocated zero-terminated string with the path or ./ if the uri is just a file name
 */
VCORE_API char* get_file_path(const char* uri);
