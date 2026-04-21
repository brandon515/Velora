#pragma once

#include "defines.h"
#include "core/event.h"
#include "platform/platform.h"
#include "vmesh.h"
#include "vmaterial.h"

typedef u64 mesh_handle;

typedef struct _render_state{
  void* internal_render_state;
} render_state;

void shutdown_render_system(render_state* state);

b8 render_preframe(render_state* state);
b8 render_frame(render_state* state);
b8 render_postframe(render_state* state);

b8 resize_handler(event* newEvent, void* state);

b8 initiate_render_system(render_state* state, const char* application_name, platform_state *plat_internal_state);

b8 register_mesh(render_state* state, vmesh mesh, u64 *outHandle);
b8 register_texture(render_state* state, velora_pixels mesh, u64 *outHandle);
b8 register_material(render_state* state, vmaterial mesh, u64 *outHandle);