#pragma once

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef char b8;

#ifdef __STDC_NO_ATOMICS__
// Unsigned int types.
typedef u8 au8;
typedef u16 au16;
typedef u32 au32;
typedef u64 au64;

// Signed int types.
typedef i8 ai8;
typedef i16 ai16;
typedef i32 ai32;
typedef i64 ai64;

// Floating point types
typedef f32 af32;
typedef f64 af64;

// Boolean types
typedef b32 ab32;
typedef b8 ab8;
#else
// Unsigned int types.
typedef _Atomic(u8) au8;
typedef _Atomic(u16) au16;
typedef _Atomic(u32) au32;
typedef _Atomic(u64) au64;

// Signed int types.
typedef _Atomic(i8)ai8;
typedef _Atomic(i16) ai16;
typedef _Atomic(i32) ai32;
typedef _Atomic(i64) ai64;

// Floating point types
typedef _Atomic(f32) af32;
typedef _Atomic(f64) af64;

// Boolean types
typedef _Atomic(b32) ab32;
typedef _Atomic(b8) ab8;
#endif

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define TRUE 1
#define FALSE 0
#define NULL ((void*)0)
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define VEL_CHECK(expr)            \
  do {                             \
    if((expr) == FALSE){           \
      return FALSE;                \
    }                              \
  } while(0)

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define VPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define VPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define VPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define VPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define VPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define VPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define VPLATFORM_IOS 1
#define VPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define VPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

// Shared library symbol visibility. Don't use these directly on declarations;
// use the per-library macros (VCORE_API, VECS_API, VRENDER_API,
// VRENDER_BACKEND_API), which pick VEXPORT or VIMPORT for the library being built.
// Keyed on the platform rather than the compiler so MinGW gets dllexport too.
#if VPLATFORM_WINDOWS
#define VEXPORT __declspec(dllexport)
#define VIMPORT __declspec(dllimport)
#else
// Libraries build with -fvisibility=hidden, so only annotated symbols are exported.
#define VEXPORT __attribute__((visibility("default")))
#define VIMPORT
#endif

// Engine specific defines
#define VELORA_MAX_OBJECTS 256
#define VELORA_MAX_MESHES 256
#define VELORA_MAX_MATERIALS 256
#define VELORA_MAX_JOINTS 256
#define VELORA_TEXTURES_PER_MATERIAL 1
#define VELORA_MAX_TEXTURES (VELORA_MAX_MATERIALS * VELORA_TEXTURES_PER_MATERIAL)