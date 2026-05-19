
# --- USER SETTINGS ---
# Change this variable to customize your plugin's name
plugin_name = 'VisiTIRF'
# ---------------------

# Enforce 64-bit build

# Ensure the equipment name macro is available to all subprojects (especially ImagerPluginCore)

# Fetch the core Imager plugin support files
imager_core_dep = dependency('imagerplugincore')

# Compile the plugin API directly into this plugin
imager_core_sp = subproject('imagerplugincore')
imager_api_source = imager_core_sp.get_variable('api_source')

# Add an include dir so that we can use ImagerPluginCore/ syntax to access its headers
core_inc = include_directories('.')

# Define the shared library (DLL) target for the plugin
shared_library(plugin_name,
  # Add your plugin source files here
  'src/VisiTIRFConnectionHandler.cpp',
  'src/VisiTIRFPlugin.cpp',

  imager_api_source,
  
  include_directories : core_inc,
  #link_with : [core_lib],
  dependencies : [imager_core_dep],
  name_suffix : 'imagerplugin', # Change the output file extension
  cpp_args : ['-DNOMINMAX', '-DIMAGER_EQUIPMENT_NAME="' + plugin_name + '"'],
  )
