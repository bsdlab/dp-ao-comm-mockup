project('ao_comm', 'cpp', default_options : ['cpp_std=c++23'])

# define the lsl.lib as dependencies
# py_dep = dependency('python', method: 'auto')
# lsl_dep = dependency('liblsl', method: 'auto')

inc_dir = include_directories(
  './ao_comm_deps/', 
  # 'C:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/ucrt'   # This is needed for ws2_32 (for socket communication under windows, potentially needs adjustment on your machine)
  )

# get the lib files for lsl and AO, paths relative to the ./build dir
lib_paths = [
    # '../ao_comm_deps/lsl/lib/lsl.lib',              # for Windows
    '/opt/homebrew/Cellar/lsl/1.16.2/lib/liblsl.dylib',              # for MacOS - if installed with `brew`, use `brew info lsl` to find dir
    # '../ao_comm_deps/AO/lib/wpcap.lib',             # not from AO but required by the AO SDK
    # '../ao_comm_deps/AO/lib/Packet.lib',            # not from AO but required by the AO SDK
    # '../ao_comm_deps/AO/lib/ethernetStandAlone_X64.lib'   # the AO SDK
  ]

custom_lib_dep = declare_dependency(
  link_args: lib_paths,
  include_directories: inc_dir
)

# Windows dependencies
compiler = meson.get_compiler('cpp')
# w_dep = compiler.find_library('ws2_32', required: true)

executable(
  meson.project_name(),
  [
    'ao_comm/main.cpp',
    'ao_comm/ao_to_lsl.cpp',
    'ao_comm/tcp_server.cpp'
    ],
  include_directories: inc_dir,
  # dependencies: [custom_lib_dep, w_dep],
  dependencies: [custom_lib_dep],
  override_options: ['cpp_std=c++23']       # default options did not take the version for me
)

