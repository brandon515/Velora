subdir('shaders')
out_render_files = files('velora_vulkan_render.c', 'vma_usage.cpp')

cc = meson.get_compiler('c')
cpp_dep = cc.find_library('stdc++',required:true)

vulkan_deps = [dependency('vulkan', required : true), dependency('VulkanMemoryAllocator', required : true), cpp_dep]

out_vulkan_lib = library('vulkan_render',
  out_render_files,
  link_with: [out_ecs_lib, out_core_lib],
  dependencies: [platform_deps, vulkan_deps],
  include_directories: inc,
  pic: true,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  cpp_args: ['-Wno-nullability-completeness', '-g', '-std=c++23'],
  install: true,
  install_dir: meson.project_source_root() / 'bin' / 'plugins'
)