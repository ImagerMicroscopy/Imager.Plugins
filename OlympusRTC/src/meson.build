project('OlympusRTC', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20', 'default_library=shared'])

cc = meson.get_compiler('cpp')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG', language: 'cpp')
endif

pugixml = dependency('pugixml')
ws2_32 = cc.find_library('ws2_32', required: true)


library('OlympusRTC',
        'OlympusRTC.cpp',
        'RTC.cpp',
        'RTCConnectionHandler.cpp',
        'SCMsgParsing.cpp',
        cpp_args : ['-DNOMINMAX'],
        name_suffix : 'imagerplugin',
        dependencies: [pugixml, ws2_32],
        install : true
)
