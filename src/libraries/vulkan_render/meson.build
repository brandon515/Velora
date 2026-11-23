subdir('shaders')
out_render_files = files('velora_vulkan_render.c', 'vma_usage.cpp')

cc = meson.get_compiler('c')
cpp_dep = cc.find_library('stdc++',required:true)

vulkan_deps = [dependency('vulkan', required : true), dependency('VulkanMemoryAllocator', required : true), cpp_dep]

# Built as a shared library. Only functions marked with this library's API
# macro are exported (see src/common/*/*_api.h); -DVELORA_RENDER_BACKEND_BUILD makes that
# macro export here and import in every other target.
out_vulkan_lib = shared_library('vulkan_render',
  out_render_files,
  link_with: [out_ecs_lib, out_assets_lib, out_core_lib],
  dependencies: [platform_deps, vulkan_deps],
  include_directories: inc,
  gnu_symbol_visibility: 'hidden',
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_RENDER_BACKEND_BUILD'],
  cpp_args: ['-DVELORA_RENDER_BACKEND_BUILD', '-Wno-nullability-completeness', '-g', '-std=c++23'],
  link_args: ['-g'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)