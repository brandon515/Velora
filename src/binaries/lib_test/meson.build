out_src_files = files('main.c')

executable('lib_test', 
  out_src_files,
  link_with: [out_core_lib, out_ecs_lib, out_render_lib, out_vulkan_lib],
  include_directories: inc,
  link_language: 'c',
  install_rpath: './plugins/',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin'
)