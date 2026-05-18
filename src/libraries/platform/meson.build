if host_machine.system() == 'linux'
  out_platform_files = files('platform_linux.c')
  platform_deps = dependency('X11', required: true)
  lib_deps += platform_deps
elif host_machine.system() == 'windows'
  out_platform_files = files('platform_win32.c')
endif

out_platform_lib = library('velora_platform',
  out_platform_files,
  dependencies: platform_deps,
  include_directories: inc,
  pic: true,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)