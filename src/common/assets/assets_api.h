#pragma once

#include "defines.h"

// Marks a function as part of the public API of the velora_assets shared library.
// Meson passes -DVELORA_ASSETS_BUILD only when compiling velora_assets's own sources, so the
// declaration exports the symbol there and imports it everywhere else.
#ifdef VELORA_ASSETS_BUILD
#define VASSETS_API VEXPORT
#else
#define VASSETS_API VIMPORT
#endif
