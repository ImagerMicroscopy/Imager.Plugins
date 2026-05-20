
# --- USER SETTINGS ---
# Change this variable to customize your plugin's name
plugin_name = 'NikonTiE'
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

# Path to the Nikon DLL so the compiler can `#import` it from any working dir
nikondll_path = meson.current_source_dir() + '/lib/NikonTi.dll'

# Define the shared library (DLL) target for the plugin
shared_library(plugin_name,
  'src/NikonTiEPlugin.cpp',
  'src/NikonTiE.cpp',

  imager_api_source,
  
  include_directories : [core_inc],
  #link_with : [core_lib],
  dependencies : [imager_core_dep],
  name_suffix : 'imagerplugin', # Change the output file extension
  cpp_args : ['-DNOMINMAX', '-DIMAGER_EQUIPMENT_NAME="' + plugin_name + '"'],
  )
