if host_machine.system() == 'linux'
  out_platform_files = files('platform_linux.c')
  platform_deps = dependency('X11', required: true)
elif host_machine.system() == 'windows'
  out_platform_files = files('platform_win32.c')
endif