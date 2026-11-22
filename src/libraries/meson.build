lib_deps = []

subdir('core')
subdir('ecs')
# vulkan_render (the backend) must come before render: the render frontend
# calls into the backend, so velora_render links against vulkan_render.
subdir('vulkan_render')
subdir('render')
# Static library providing main(); linked into executables. See entry/meson.build.
subdir('entry')
