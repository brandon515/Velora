#include "render/velora_render.h"
#include "core/event.h"

void shutdown_render_system(render_state* state){
  deregister_listener(state->mesh_listener_handle);
  deregister_listener(state->material_listener_handle);
  deregister_listener(state->texture_listener_handle);
  shutdown_render_backend(state->internal_render_state);
}

b8 initiate_render_system(render_state* state, const char* application_name, platform_state *plat_internal_state){
  VEL_CHECK(initiate_render_backend(&state->internal_render_state, application_name, plat_internal_state));
  state->material_listener_handle = register_listener(ENGINE_REGISTER_MATERIAL, register_material_handler, state->internal_render_state);
  state->texture_listener_handle = register_listener(ENGINE_REGISTER_TEXTURE, register_texture_handler, state->internal_render_state);
  state->mesh_listener_handle = register_listener(ENGINE_REGISTER_MESH, register_mesh_handler, state->internal_render_state);
  return TRUE;
}
