project('ImagerPlugin', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20']
)

# --- USER SETTINGS ---
# Change this variable to customize your plugin's name
plugin_name = 'ImagerPlugin'
# ---------------------

# Enforce 64-bit build
if host_machine.cpu_family() != 'x86_64'
  error('This plugin must be built for 64-bit architecture. Please configure your build system for x64.')
endif

# Ensure the equipment name macro is available to all subprojects (especially ImagerPluginCore)
add_global_arguments('-DIMAGER_EQUIPMENT_NAME="' + plugin_name + '"', language : 'cpp')

# Fetch the core Imager plugin support files
imager_core_dep = dependency('imagerplugincore')

# Define the shared library (DLL) target for the plugin
shared_library(plugin_name,
  'src/DummyEquipmentPlugin.cpp',
  'src/DummyEquipment.cpp',
  
  include_directories : [],
  dependencies : [imager_core_dep],
  name_suffix : 'imagerplugin', # Change the output file extension
  cpp_args : ['-DNOMINMAX'],
)
