#pragma once

#include "defines.h"

// Marks a function as part of the public API of the velora_core shared library.
// Meson passes -DVELORA_CORE_BUILD only when compiling velora_core's own sources, so the
// declaration exports the symbol there and imports it everywhere else.
#ifdef VELORA_CORE_BUILD
#define VCORE_API VEXPORT
#else
#define VCORE_API VIMPORT
#endif
