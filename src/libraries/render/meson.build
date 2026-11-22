out_render_files = files('vmaterial.c', 'vmesh.c', 'velora_render.c')

# Built as a shared library. Only functions marked with this library's API
# macro are exported (see src/common/*/*_api.h); -DVELORA_RENDER_BUILD makes that
# macro export here and import in every other target.
out_render_lib = shared_library('velora_render',
  out_render_files,
  link_with: [out_vulkan_lib, out_core_lib],
  include_directories: inc,
  gnu_symbol_visibility: 'hidden',
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_RENDER_BUILD'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)