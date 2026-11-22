#pragma once

#include "defines.h"

// Marks a function as part of the public API of the velora_ecs shared library.
// Meson passes -DVELORA_ECS_BUILD only when compiling velora_ecs's own sources, so the
// declaration exports the symbol there and imports it everywhere else.
#ifdef VELORA_ECS_BUILD
#define VECS_API VEXPORT
#else
#define VECS_API VIMPORT
#endif
