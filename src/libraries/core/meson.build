out_core_files = files('event.c', 'input.c', 'logger.c', 'vmemory.c', 'stb_image.c')
subdir('utils')
subdir('container')
out_core_files += out_utils_files
out_core_files += out_containers_files

if host_machine.system() == 'linux'
  cc = meson.get_compiler('c')
  m_dep = cc.find_library('m', required : true)
endif

out_core_lib = library('velora_core',
  out_core_files,
  link_with: out_platform_lib,
  dependencies: m_dep,
  include_directories: inc,
  pic: true,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)