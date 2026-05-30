out_ecs_files = files('vcamera.c', 'vecs.c', 'vrenderable.c', 'vtransform.c', 'vskin.c')

out_ecs_lib = library('velora_ecs',
  out_ecs_files,
  link_with: out_core_lib,
  include_directories: inc,
  pic: true,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)