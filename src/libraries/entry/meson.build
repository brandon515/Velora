out_entry_files = files('entry.c')

# entry.c defines main() and calls into the core, render, ecs and application
# layers. Building it as a static library defers symbol resolution to the final
# executable link, so it doesn't need to (and can't, since the application's
# callbacks live in the executable) resolve them itself.
out_entry_lib = static_library('velora_entry',
  out_entry_files,
  dependencies: [platform_deps],
  include_directories: inc,
  c_args: ['-g', '-Wvarargs', '-Wall', '-Werror'],
  install: false
)
