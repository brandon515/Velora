out_render_files = files('vmaterial.c', 'vmesh.c')

out_render_lib = library('velora_render',
  out_render_files,
  link_with: out_core_lib,
  include_directories: inc,
  pic: true,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)