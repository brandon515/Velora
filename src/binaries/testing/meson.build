out_test_files = files('test_entry.c', 'darray_test.c', 'vmemory_test.c')

# Unit tests compile velora_core's sources straight in, so they
# don't depend on the entry point or renderer and run on CI machines with no GPU.
# VELORA_CORE_BUILD is defined because the core sources are compiled into this
# executable rather than imported from the DLL.
velora_tests = executable('velora_tests',
  out_test_files + out_core_files,
  dependencies: [m_dep, platform_deps],
  include_directories: inc,
  link_language: 'c',
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_CORE_BUILD'],
  link_args: ['-g'],
  install: false
)

foreach suite : ['vmemory', 'darray']
  test(suite, velora_tests, args: [suite], timeout: 30)
endforeach
