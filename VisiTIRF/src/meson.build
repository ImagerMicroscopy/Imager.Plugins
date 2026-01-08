project('VisiTIRF', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20', 'default_library=shared'])

cc = meson.get_compiler('cpp')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG', language: 'cpp')
endif


library('VisiTIRF',
        'VisiTIRF.cpp',
        'VisiTIRFConnectionHandler.cpp',
        cpp_args : ['-DNOMINMAX'],
        name_suffix : 'imagerplugin',
        install : true
)
