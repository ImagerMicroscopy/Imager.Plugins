project('MicroscopeControl', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20', 'default_library=shared'])

# --- 1. Path Setup ---
# Since we are in /src, we go up one level to reach /include and /lib
inc_base = include_directories('../include')
inc_ix83 = include_directories('../include/OlympusIX83')
inc_ti2  = include_directories('../include/NikonTi2')
inc_src  = include_directories('.') # For Config.h and local headers

# Path to the pre-compiled .lib files
abslibdir = join_paths(meson.current_source_dir(), '../lib/x64')

# --- 2. Configuration & Header Generation ---
conf_data = configuration_data()
m_type = get_option('microscope_type')

conf_data.set('WITH_TI1', m_type == 'nikon_ti1')
conf_data.set('WITH_TI2', m_type == 'nikon_ti2')
conf_data.set('WITH_IX83', m_type == 'olympus_ix83')
conf_data.set('WITH_DUMMYMICROSCOPE', m_type == 'dummy')
conf_data.set('WITH_DONOTHINGMICROSCOPE', m_type == 'do_nothing')

configure_file(input : 'Config.h.in',
               output : 'Config.h',
               configuration : conf_data)

# --- 3. Dependency Logic ---
cc = meson.get_compiler('cpp')
libs = []
all_inc = [inc_base, inc_src]

if m_type == 'ti1'
    libs += cc.find_library('NikonTi', dirs: abslibdir)
elif m_type == 'ti2'
    libs += cc.find_library('Ti2_Mic_Driver', dirs: abslibdir)
    all_inc += inc_ti2
elif m_type == 'ix83'
    # No need for specific IX83 libs
    all_inc += inc_ix83
endif

# --- 4. Build Target ---
cpp_flags = ['-DNOMINMAX']
if get_option('buildtype') == 'debug'
    cpp_flags += '-DDEBUG'
endif

# Only include the specific files needed for the library
# Note: since the .cpp files include content only when #ifdef'd including all
# of them in the src_files won't increase the binary file
src_files = [
    'BaseMicroscopeClass.cpp',
    'DoNothingMicroscope.cpp',
    'DummyMicroscope.cpp',
    'MicroscopeControlDLL.cpp',
    'NikonTi1.cpp',
    'NikonTi2.cpp',
    'OlympusIX83.cpp'
]

library('MicroscopeControl',
    src_files,
    include_directories: all_inc,
    dependencies : libs,
    cpp_args : cpp_flags,
    name_suffix : 'imagerplugin',
    install : true
)