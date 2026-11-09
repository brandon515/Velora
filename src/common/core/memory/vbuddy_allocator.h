#pragma once
#include "defines.h"
#include "core/container/darray.h"

typedef struct _vbuddy_block{
  u64 id; // The id of thing that has allocated this node.
  u64 offset; // The actual offset in memory for this node.
}vbuddy_block;

typedef struct _vbuddy_node{
  struct _vbuddy_node* parent;
  struct _vbuddy_node* left;
  struct _vbuddy_node* right;
  vbuddy_block memory;
}vbuddy_node;

typedef struct _vbuddy_array_item{
  vbuddy_node *node;
}vbuddy_array_item;

typedef struct _vbuddy_allocator{
  u64 maxOrder; // The maximum power of 2. This should encompass the entire memory space as much as possible
  u64 minOrder; // The minimum power of 2.
  vbuddy_node rootNode; // The root of the binary tree
  darray* free_lists[64]; // An array of dynamic arrays of free buddy nodes by order (ie 2^3 blocks are in the darray in index 3)
  darray* allocated_lists[64]; // An array of dynamic arrays of buddy nodes that have been allocated 
}vbuddy_allocator;

b8 create_buddy_allocator(u64 initialOffset, u64 memorySize, u64 minOrder, vbuddy_allocator *outAlloc);

void free_buddy_allocator(vbuddy_allocator* allocator);

vbuddy_node* create_node(vbuddy_allocator* allocator, u64 order);

void free_node(vbuddy_node* node);

u64 buddy_allocate(vbuddy_allocator* allocator, u64 id, u64 size);

void buddy_free(vbuddy_allocator* allocator, u64 id);