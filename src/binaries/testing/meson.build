# Every suite listed here is also registered in test_entry.c's suites[] table.
test_suites = [
  'vmemory', 'darray', 'map', 'event', 'vstring', 'vmath',
  'vfile', 'vjson', 'vgltf', 'logger', 'platform', 'ecs', 'entry',
]

out_test_files = files('test_entry.c', 'test_framework.c')
foreach suite : test_suites
  out_test_files += files(suite + '_test.c')
endforeach

# Unit tests compile velora_core's, velora_assets's and velora_ecs's sources,
# and the engine's main loop (entry_loop.c), straight in, so they don't depend on the entry point or renderer and run on CI
# machines with no GPU. VELORA_CORE_BUILD, VELORA_ASSETS_BUILD and
# VELORA_ECS_BUILD are defined because those sources are compiled into this
# executable rather than imported from the DLLs.
# RUN_TEST_KNOWN_BUG_SANITIZER tests only run in an ASan/UBSan build. The
# option is a string ('none') before Meson 1.8 and an array ([]) after.
test_c_args = ['-g', '-Wvarargs', '-Wall', '-Werror', '-DVELORA_CORE_BUILD', '-DVELORA_ASSETS_BUILD', '-DVELORA_ECS_BUILD']
if '@0@'.format(get_option('b_sanitize')) not in ['none', '[]', '']
  test_c_args += '-DVELORA_TEST_SANITIZERS'
endif

velora_tests = executable('velora_tests',
  out_test_files + out_core_files + out_assets_files + out_ecs_files + out_entry_loop_files,
  dependencies: [m_dep, platform_deps],
  include_directories: inc,
  link_language: 'c',
  c_args: test_c_args,
  link_args: ['-g'],
  install: false
)

# Each suite is its own meson test, so one failing or crashing suite doesn't
# hide the others. --failures keeps the log to the tests that failed and the
# summary. The per-test-case timeout lives in test_framework.c.
foreach suite : test_suites
  test(suite, velora_tests, args: ['--failures', suite], timeout: 120)
endforeach
