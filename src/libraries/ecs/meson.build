out_ecs_files = files('vcamera.c', 'vecs.c', 'vrenderable.c', 'vtransform.c', 'vskin.c')

# Built as a shared library. Only functions marked with this library's API
# macro are exported (see src/common/*/*_api.h); -DVELORA_ECS_BUILD makes that
# macro export here and import in every other target.
out_ecs_lib = shared_library('velora_ecs',
  out_ecs_files,
  link_with: [out_assets_lib, out_core_lib],
  include_directories: inc,
  gnu_symbol_visibility: 'hidden',
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_ECS_BUILD'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)