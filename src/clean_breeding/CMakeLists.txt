project(unnatural_selection)

# Development tools (simulator, test suite, snapshots, Debug menu, [debug] test helpers). OFF for the release build.
option(CB_DEV_TOOLS "Build the development tools" ON)
if(CB_DEV_TOOLS)
    set(CB_DEV_VALUE 1)
else()
    set(CB_DEV_VALUE 0)
endif()

# List build targets
add_library(${PROJECT_NAME} SHARED
    amoeboid.cpp
    amoeboid.def
    clean_breeding.cpp
    breed.cpp
    cat_factory.cpp
    config.cpp
    menu.cpp
    gamelook/assets.cpp
    gamelook/fontloader.cpp
    gamelook/gamelook.cpp
    gamelook/gpak.cpp
    gamelook/swf.cpp
    utilities/mewjector_support.c
    utilities/pe_view.cpp
)

if(CB_DEV_TOOLS)
    target_sources(${PROJECT_NAME} PRIVATE simulator.cpp snapshot.cpp)
endif()

# List dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    Detours
    dear_imgui
    Mewgenics_exports
    libtomcrypt
    mewjector
    SDL3::Headers
    opengl32
)

# List include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gamelook
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/stb
)

# List definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    CB_DEV_TOOLS=${CB_DEV_VALUE}
)

# Set required flags
if(MSVC)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Statically link libc++ (static-libstdc++ flag also covers libc++)
        target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++)
        # Generate a PDB
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--pdb=)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Statically link libgcc/libstdc++/libwinpthread
        target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
    endif()
    # Don't prefix the hook dll with "lib" in its filename
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

# Set language modes
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# Set warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set install paths
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION . OPTIONAL)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        install(FILES "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb" DESTINATION . OPTIONAL)
    endif()
endif()

# Code ported from Combat Roster Panel keeps its own warning level and needs UTF-8 sources.
if(MSVC)
    set_source_files_properties(gamelook/gpak.cpp gamelook/swf.cpp gamelook/fontloader.cpp gamelook/assets.cpp
        PROPERTIES COMPILE_OPTIONS "/W3;/utf-8")
endif()
