#pragma once

#include "defines.h"

// Marks a function as part of the public API of the velora_render shared library.
// Meson passes -DVELORA_RENDER_BUILD only when compiling velora_render's own sources, so the
// declaration exports the symbol there and imports it everywhere else.
#ifdef VELORA_RENDER_BUILD
#define VRENDER_API VEXPORT
#else
#define VRENDER_API VIMPORT
#endif
