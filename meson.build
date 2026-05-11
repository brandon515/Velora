project('Velora', 'c', 'cpp')

deps = [dependency('vulkan', required : true), dependency('VulkanMemoryAllocator', required : true)]

cc = meson.get_compiler('c')
cpp_dep = cc.find_library('stdc++',required:true)
deps += cpp_dep

if host_machine.system() == 'linux'
  m_dep = cc.find_library('m', required : true)
  deps += [dependency('X11', required: true), m_dep]
endif

inc = include_directories('./src/')

subdir('src')

add_project_arguments('-D_DEBUG', language: 'c')
add_project_arguments('-DVULKAN_RENDER', language: 'c')
add_project_arguments('-D_CRT_SECURE_NO_WARNINGS', language: 'c')

executable('engine', 
  out_src_files, 
  dependencies: deps, 
  include_directories: inc, 
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  cpp_args: ['-Wno-nullability-completeness', '-g', '-std=c++23'],
  install: true,
  install_dir: meson.project_source_root() / 'bin'
)

  