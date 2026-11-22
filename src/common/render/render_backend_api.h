#pragma once

#include "defines.h"

// Marks a function as part of the public API of the the render backend (vulkan_render) shared library.
// Meson passes -DVELORA_RENDER_BACKEND_BUILD only when compiling the render backend (vulkan_render)'s own sources, so the
// declaration exports the symbol there and imports it everywhere else.
#ifdef VELORA_RENDER_BACKEND_BUILD
#define VRENDER_BACKEND_API VEXPORT
#else
#define VRENDER_BACKEND_API VIMPORT
#endif
