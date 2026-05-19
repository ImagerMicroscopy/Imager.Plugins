
# --- USER SETTINGS ---
# Change this variable to customize your plugin's name
plugin_name = 'DummyEquipment'
# ---------------------

# Enforce 64-bit build

# Ensure the equipment name macro is available to all subprojects (especially ImagerPluginCore)

# Fetch the core Imager plugin support files
imager_core_dep = dependency('imagerplugincore')

# Compile the plugin API directly into this plugin
imager_core_sp = subproject('imagerplugincore')
imager_api_source = imager_core_sp.get_variable('api_source')

# Define the shared library (DLL) target for the plugin
shared_library(plugin_name,
  'src/DummyEquipmentPlugin.cpp',
  'src/DummyEquipment.cpp',

  imager_api_source,
  
  include_directories : [],
  dependencies : [imager_core_dep],
  name_suffix : 'imagerplugin', # Change the output file extension
  name_prefix : '',
  cpp_args : ['-DNOMINMAX', '-DIMAGER_EQUIPMENT_NAME="' + plugin_name + '"'],
)
