out_core_files = files('event.c', 'logger.c', 'stb_image.c')
subdir('utils')
subdir('container')
subdir('platform')
subdir('memory')
out_core_files += out_utils_files
out_core_files += out_containers_files
out_core_files += out_platform_files
out_core_files += out_memory_files

m_dep = []
if host_machine.system() == 'linux'
  cc = meson.get_compiler('c')
  m_dep = cc.find_library('m', required : true)
endif

# Built as a shared library. Only functions marked with this library's API
# macro are exported (see src/common/*/*_api.h); -DVELORA_CORE_BUILD makes that
# macro export here and import in every other target.
out_core_lib = shared_library('velora_core',
  out_core_files,
  dependencies: [m_dep, platform_deps],
  include_directories: inc,
  gnu_symbol_visibility: 'hidden',
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_CORE_BUILD'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)