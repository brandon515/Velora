out_game_files = files('entry.c', 'application.c')

executable('engine', 
  out_game_files,
  link_with: [out_platform_lib, out_core_lib, out_ecs_lib, out_render_lib, out_vulkan_lib],
  include_directories: inc,
  link_language: 'c',
  install_rpath: './plugins/',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin'
)