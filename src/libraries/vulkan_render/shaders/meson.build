# Find the shader compiler (glslc is recommended for better error messages)
glslc = find_program('glslc', required : true)

shader_filenames = ['frag.glsl', 'vert.glsl']

out_shader_target = []

foreach file : shader_filenames
  out_shader_target += custom_target(
    file+'_spv',
    input : files(file),
    output : file+'.spv',
    command : [glslc, '@INPUT@', '-o', '@OUTPUT@'],
    install: true,
    install_dir: meson.project_source_root() / 'bin'
  )
endforeach
