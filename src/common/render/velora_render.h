#pragma once

#include "core/utils/vimage.h"
#include "defines.h"
#include "core/event.h"
#include "core/platform/platform.h"
#include "render/vmesh.h"
#include "render/vmaterial.h"

typedef u64 mesh_handle;

typedef struct _render_state{
  void* internal_render_state;
} render_state;

typedef struct _register_mesh_data{
  vmesh mesh;
  u64 handle;
}register_mesh_data;

typedef struct _register_texture_data{
  velora_pixels pixels;
  u64 handle;
}register_texture_data;

typedef struct _register_material_data{
  vmaterial material;
  u64 handle;
}register_material_data;

void shutdown_render_system(render_state* state);

b8 initiate_render_system(render_state* state, const char* application_name, platform_state *plat_internal_state);

b8 register_mesh(vmesh mesh, u64 handle);
b8 register_texture(velora_pixels mesh, u64 handle);
b8 register_material(vmaterial mesh, u64 handle);

//The functions below are to be defined in the specific backend implementation

b8 initiate_render_backend(void** internalState, const char* application_name, platform_state *plat_internal_state);
void shutdown_render_backed(void* internalState);

b8 regiser_mesh_handler(event* event, void* state);
b8 regiser_texture_handler(event* event, void* state);
b8 regiser_material_handler(event* event, void* state);
b8 resize_handler(event* newEvent, void* state);

b8 render_preframe(render_state* state);
b8 render_frame(render_state* state);
b8 render_postframe(render_state* state);