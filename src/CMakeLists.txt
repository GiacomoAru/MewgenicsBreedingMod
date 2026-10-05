cmake_minimum_required(VERSION 3.15)

project(amoeboid)

# Set global compile options
if(MSVC)
    # append "-- /p:CL_MPcount=<n>" to "cmake --build" to control TU-level parallelism
    add_compile_options(/MP)
endif()

# Read third-party library CMakeLists
add_subdirectory(lib/Detours EXCLUDE_FROM_ALL)
add_subdirectory(lib/imgui EXCLUDE_FROM_ALL)
add_subdirectory(lib/Mewgenics EXCLUDE_FROM_ALL)
add_subdirectory(lib/libtomcrypt EXCLUDE_FROM_ALL)
add_subdirectory(lib/mewjector EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Read our CMakeLists
add_subdirectory(clean_breeding)
add_executable(test_config EXCLUDE_FROM_ALL tests/test_config.cpp)
target_include_directories(test_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(test_config PROPERTIES CXX_STANDARD 20)
add_executable(test_breed_logic EXCLUDE_FROM_ALL tests/test_breed_logic.cpp)
target_include_directories(test_breed_logic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(test_breed_logic PROPERTIES CXX_STANDARD 20)
