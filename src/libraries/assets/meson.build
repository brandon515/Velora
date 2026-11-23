out_assets_files = files('vjson.c', 'vgltf.c', 'vimage.c', 'stb_image.c')

# Asset loaders (JSON, glTF, images). Built as a shared library on top of
# velora_core. Only functions marked with this library's API macro are exported
# (see src/common/*/*_api.h); -DVELORA_ASSETS_BUILD makes that macro export here
# and import in every other target. See docs/adr/0002-asset-loaders-in-their-own-library.md.
out_assets_lib = shared_library('velora_assets',
  out_assets_files,
  link_with: out_core_lib,
  dependencies: [m_dep],
  include_directories: inc,
  gnu_symbol_visibility: 'hidden',
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_ASSETS_BUILD'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)
