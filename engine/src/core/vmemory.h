#pragma once

#include "defines.h"

#define FOREACH_MEM_TAG(MEM_TAG)          \
  MEM_TAG(MEMORY_TAG_UNKNOWN)             \
  MEM_TAG(MEMORY_TAG_ARRAY)               \
  MEM_TAG(MEMORY_TAG_DARRAY)              \
  MEM_TAG(MEMORY_TAG_DICT)                \
  MEM_TAG(MEMORY_TAG_RING_QUEUE)          \
  MEM_TAG(MEMORY_TAG_BST)                 \
  MEM_TAG(MEMORY_TAG_STRING)              \
  MEM_TAG(MEMORY_TAG_APPLICATION)         \
  MEM_TAG(MEMORY_TAG_JOB)                 \
  MEM_TAG(MEMORY_TAG_TEXTURE)             \
  MEM_TAG(MEMORY_TAG_MATERIAL_INSTANCE)   \
  MEM_TAG(MEMORY_TAG_RENDERER)            \
  MEM_TAG(MEMORY_TAG_GAME)                \
  MEM_TAG(MEMORY_TAG_TRANSFORM)           \
  MEM_TAG(MEMORY_TAG_ENTITY)              \
  MEM_TAG(MEMORY_TAG_ENTITY_NODE)         \
  MEM_TAG(MEMORY_TAG_SCENE)               \
  MEM_TAG(MEMORY_TAG_INT_MAP)             \
  MEM_TAG(MEMORY_TAG_EVENT_DATA)          \
  MEM_TAG(MEMORY_TAG_INPUT_DATA)          \
  MEM_TAG(MEMORY_TAG_IMAGE)               \
  MEM_TAG(MEMORY_TAG_JSON)                \
  MEM_TAG(MEMORY_TAG_END_TAG)             \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum _memory_tag {
  FOREACH_MEM_TAG(GENERATE_ENUM)
} memory_tag;

static const char* memory_tag_strings[] ={
  FOREACH_MEM_TAG(GENERATE_STRING)
};

VAPI void initialize_memory();
VAPI void shutdown_memory();

VAPI void* vallocate(u64 size, memory_tag tag);
VAPI void vfree(void* block, u64 size, memory_tag tag);
VAPI void* vzero_memory(void* block, u64 size);
VAPI void* vcopy_memory(void* dest, void* src, u64 size);
VAPI void* vset_memory(void* block, i32 value, u64 size);

VAPI char* get_memory_usage_str();