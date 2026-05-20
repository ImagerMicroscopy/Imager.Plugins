project('ImagerPlugins', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20']
)

# Enforce 64-bit build
if host_machine.cpu_family() != 'x86_64'
  error('These plugins must be built for 64-bit architecture.')
endif

subdir('DummyCamera')
subdir('DummyEquipment')
subdir('Lambda10B')
subdir('MarzhauserTango')
subdir('NikonTi2')
subdir('NikonTiE')
subdir('OlympusIX83')
subdir('OlympusRTC')
subdir('OxxiusCombiner')
subdir('PriorProScanIII')
subdir('VisiTIRF')
