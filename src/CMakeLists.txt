cmake_minimum_required(VERSION 3.15)

project(amoeboid)

# Set global compile options
if(MSVC)
    # append "-- /p:CL_MPcount=<n>" to "cmake --build" to control TU-level parallelism
    add_compile_options(/MP)
endif()

# Read third-party library CMakeLists
add_subdirectory(lib/Detours EXCLUDE_FROM_ALL)
add_subdirectory(lib/libtomcrypt EXCLUDE_FROM_ALL)
add_subdirectory(lib/mewjector EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Read our CMakeLists
add_subdirectory(clean_breeding)
